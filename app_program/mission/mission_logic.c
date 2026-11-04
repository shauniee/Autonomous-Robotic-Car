#include "mission_logic.h"
#include "../common/robot_time_math.h"
#include "../motion/motion.h"
#include <math.h>
#include <stdlib.h>

#define LAST_ACTION_ID rd_mask(RD_MOTION_ACTION_BITS)

static bool obstacle_ahead(const MissionInputs *in);
static bool obstacle_in_stopping_range(const MissionInputs *in);
static void handle_scan(Mission *m, const MissionInputs *in,
    MissionOutputs *out);
static void handle_reacquire(Mission *m, const MissionInputs *in,
    MissionOutputs *out);
static void enter_bypass(Mission *m, const MissionInputs *in,
    MissionOutputs *out);
static void handle_junction_turn(Mission *m, const MissionInputs *in,
    MissionOutputs *out);

void mission_init(Mission *m, uint32_t now_ms)
{
    *m = (Mission){.state = MISSION_READY, .state_since_ms = now_ms};
}

static float mission_turn_target_get(const Mission *m)
{
    switch (m->turn_code)
    {
    case 1u:
        return 90.0f;
    case 2u:
        return -90.0f;
    case 3u:
        return -180.0f;
    default:
        return 0.0f;
    }
}

static void mission_turn_target_set(Mission *m, barcode_action_t action)
{
    m->turn_code =
        action == BARCODE_LEFT
    ? 1u
    : (action == BARCODE_RIGHT ? 2u
        : (action == BARCODE_U_TURN ? 3u : 0u));
}

static void enter(Mission *m, MissionState next, uint32_t now_ms)
{
    m->state = next;
    m->state_since_ms = now_ms;
    m->line_steer_q = 0;
    m->line_side = 0;
    m->line_prev = 0;
    m->line_bend = 0u;
    m->line_crossed = 0u;
    m->line_steer_sampled_ms = now_ms - IR_MAX_GAP_MS - 1u;
}

static float travel_mm(const MissionInputs *in, const enc_count_t origin[2])
{
    return enc_mean_travel(in->wheel_count, origin) / ENC_COUNTS_PER_MM;
}

static void mark(enc_count_t origin[2], const MissionInputs *in)
{
    origin[0] = in->wheel_count[0];
    origin[1] = in->wheel_count[1];
}

/* Steering sign follows the mounted robot's observed turn direction.
   Distance-based P: no integral, so nothing carries into the next curve or
   crossing. A touch steers hard at once; a bend is the same side touching again
   soon or staying black long, and the other side touching ends it (S-bends);
   white/white drifts toward the last side so a centred robot soon touches again. */
static int line_digital_steering(Mission *m, const MissionInputs *in)
{
    if (in->usable_line && in->guidance_valid &&
        in->line_sampled_ms != m->line_steer_sampled_ms)
    {
        int sign = in->line_error < 0 ? -1 : in->line_error > 0 ? 1 : 0;
        int steer;
        if (sign != 0)
        {
            if (sign != m->line_prev)
            {
                float window = m->line_bend ? LINE_BEND_LEAN_MM : LINE_BEND_TOUCH_MM;
                m->line_bend = !m->line_crossed && sign == m->line_side &&
                    travel_mm(in, m->dark_origin) <= window;
                mark(m->touch_origin, in);
            }
            /* INT-190: the first touch after a crossing is regaining the line, which
               takes longer than a straight touch; only a very long one is a bend. */
            else if (travel_mm(in, m->touch_origin) >
                (m->line_crossed ? LINE_BEND_LEAN_MM : LINE_BEND_TOUCH_MM))
            {
                m->line_bend = 1u; /* black longer than a straight touch */
            }
            m->line_side = (int8_t)sign;
            mark(m->dark_origin, in);
            steer = sign * (m->line_bend ? LINE_BEND_EDGE_Q : LINE_EDGE_Q);
        }
        else
        {
            if (m->line_prev != 0)
            {
                m->line_crossed = 0u; /* that touch has ended */
            }
            bool lean = m->line_bend &&
                travel_mm(in, m->dark_origin) <= LINE_BEND_LEAN_MM;
            steer = m->line_side * (lean ? LINE_BEND_LEAN_Q : LINE_DRIFT_Q);
        }
        m->line_prev = (int8_t)sign;
        m->line_steer_q = (int8_t)steer;
        m->line_steer_sampled_ms = in->line_sampled_ms;
    }
    return m->line_steer_q;
}

/* Where each sensor last went black, from fresh acquisitions in any state. */
static void note_onsets(Mission *m, const MissionInputs *in)
{
    if (!in->usable_line || in->line_sampled_ms == m->onset_sampled_ms)
    {
        return;
    }
    m->onset_sampled_ms = in->line_sampled_ms;
    unsigned black = (in->left_black ? 1u : 0u) | (in->right_black ? 2u : 0u);
    for (unsigned side = 0u; side < 2u; ++side)
    {
        if ((black & ~m->onset_black) >> side & 1u)
        {
            mark(m->onset[side], in);
        }
    }
    m->onset_black = (uint8_t)black;
}

/* Distance between the two sensors' black onsets, mm. A crossing darkens both
   together; curve skew darkens the inner sensor long before the outer. */
static float onset_skew_mm(const Mission *m)
{
    const uint32_t first[2] = {m->onset[0][0], m->onset[0][1]};
    return fabsf(enc_mean_travel(first, m->onset[1])) / ENC_COUNTS_PER_MM;
}

static uint8_t next_action_id(Mission *m)
{
    /* ID 1 is reserved for FOLLOW so a wrapped action ID can never alias it. */
    m->action_id = (uint8_t)(m->action_id >= LAST_ACTION_ID || m->action_id < 1u
        ? 2u
        : m->action_id + 1u);
    return m->action_id;
}

static MotionCommand stop_command(uint32_t now_ms)
{
    return motion_command_make(MOTION_MODE_STOP, 0.0f, 0.0f, 0, 0u, now_ms);
}

/* Action targets are integral mm or degrees; invalid/out-of-range values
fail closed as a STOP command rather than being truncated. */
static MotionCommand action_command(motion_mode_t mode, float speed,
    float target, uint8_t id, uint32_t now_ms)
{
    return motion_command_make(mode, floorf(speed), target, 0, id, now_ms);
}

static MotionCommand junction_turn_command(const Mission *m,
    const MissionInputs *in)
{
    /* A bound, not the goal: the pivot ends when the new branch is found. */
    float bound = m->turn_code == 3u ? NAV_UTURN_MAX_DEG : NAV_TURN_MAX_DEG;
    return motion_command_make(MOTION_MODE_TURN,
        MISSION_TRACK_SPEED_MMPS,
        mission_turn_target_get(m) < 0.0f ? -bound : bound, MOTION_TURN_RAMPED,
        m->current_action, in->line_sampled_ms);
}

static MotionCommand follow_command(const MissionInputs *in, int steering_q,
    uint8_t id)
{
    return motion_command_make(MOTION_MODE_FOLLOW, MISSION_TRACK_SPEED_MMPS, 0.0f,
        steering_q, id, in->line_sampled_ms);
}

static void to_stop(Mission *m, const MissionInputs *in, MissionOutputs *out,
    bool call_stop)
{
    enter(m, MISSION_STOPPED, in->now_ms);
    m->held = BARCODE_UNMAPPED;
    m->manual_held = 0u;
    m->in_broad = false;
    out->stop_now = call_stop;
    out->stop_scanner = true;
    out->scan_cancel = true;
    out->side_watch_on = false;
    out->command = stop_command(in->now_ms);
}

static void handle_ready(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (!in->start_pressed)
    {
        return;
    }
    if (!in->mapped || in->start_blocked || in->motion_stopped ||
        in->safety_latched || !in->guidance_valid)
    {
        out->start_refused = true;
        return;
    }
    m->in_broad = false;
    m->junction_locked = 0u;
    m->held = BARCODE_UNMAPPED;
    m->manual_held = 0u;
    enter(m, MISSION_FOLLOW_LINE, in->now_ms);
    out->start_scanner = true;
}

static void handle_stop(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    (void)out;
    /* GP21 release is serviced by safety_step; GP20 never resets. */
    if (!in->stop_down && !in->safety_latched && !in->motion_stopped &&
        in->inputs_ok)
    {
        enter(m, MISSION_READY, in->now_ms);
    }
}

static void observe_junction(Mission *m, const MissionInputs *in)
{
    if (!m->in_broad || !m->junction_open ||
        in->line_sampled_ms == m->junction_sampled_ms)
    {
        return;
    }
    m->junction_sampled_ms = in->line_sampled_ms;
    if (in->line != IR_LINE_BROAD_BLACK || !in->usable_line)
    {
        m->junction_open = 0u;
        return;
    }
    if (enc_mean_travel(in->wheel_count, m->broad_origin) >=
        NAV_CONFIRM_MM * ENC_COUNTS_PER_MM)
    {
        m->junction_qualified = 1u;
    }
}

static void handle_follow(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (m->in_broad && (!in->usable_line || obstacle_ahead(in) ||
        robot_time_elapsed(in->now_ms, m->broad_since_ms) >=
        MOTION_ACTION_TIMEOUT_MS))
    {
        to_stop(m, in, out, true);
        return;
    }
    if (in->have_barcode)
    {
        if (!m->manual_held && in->barcode_action != BARCODE_UNMAPPED)
        {
            m->held = in->barcode_action; /* the newest read replaces an unused one */
        }
        /* Report every read, so a letter missing from the mapping is visible. */
        out->event_kind = MQTT_EVENT_BARCODE;
        out->event_arg = in->barcode_letter;
        out->event_arg2 = (uint16_t)in->barcode_action;
    }

    if (obstacle_in_stopping_range(in))
    {
        enter_bypass(m, in, out);
        return;
    }
    bool broad = in->line == IR_LINE_BROAD_BLACK && in->usable_line;
    if (m->junction_locked)
    {
        if (broad)
        {
            m->broad_origin[0] = in->wheel_count[0];
            m->broad_origin[1] = in->wheel_count[1];
        }
        else if (enc_mean_travel(in->wheel_count, m->broad_origin) >=
            NAV_REARM_MM * ENC_COUNTS_PER_MM)
        {
            m->junction_locked = 0u;
        }
    }
    if (broad && !m->in_broad && !m->junction_locked)
    {
        m->in_broad = true;
        m->entry_curved = onset_skew_mm(m) > NAV_ONSET_SKEW_MM;
        m->broad_origin[0] = m->entry_origin[0] = in->wheel_count[0];
        m->broad_origin[1] = m->entry_origin[1] = in->wheel_count[1];
        m->junction_open = 1u;
        m->junction_qualified = 0u;
        m->junction_sampled_ms = in->line_sampled_ms;
        m->broad_since_ms = in->now_ms;
    }
    observe_junction(m, in);
    if (m->in_broad && !broad)
    {
        m->in_broad = false;
    }
    bool held_turn = m->held == BARCODE_LEFT || m->held == BARCODE_RIGHT ||
        m->held == BARCODE_U_TURN;
    /* Curve skew reads both black too: without a queued turn, keep following. */
    if (m->in_broad && m->junction_qualified && !m->junction_locked &&
        (held_turn || !m->entry_curved))
    {
        barcode_action_t action = m->held;
        bool turning = action == BARCODE_LEFT || action == BARCODE_RIGHT ||
            action == BARCODE_U_TURN;
        float heading = enc_heading_deg(
            enc_travel_signed(in->wheel_count[0], m->entry_origin[0]),
            enc_travel_signed(in->wheel_count[1], m->entry_origin[1]));
        /* The advance assumes the robot arrived along the line. */
        if (turning && (!NAV_GEOMETRY_RELEASED || NAV_SENSOR_FORWARD_MM <= 0 ||
            NAV_CROSSBAR_MM <= 0 || fabsf(heading) > NAV_ENTRY_MAX_DEG))
        {
            out->nav_rejected = (!NAV_GEOMETRY_RELEASED || NAV_SENSOR_FORWARD_MM <= 0 ||
                NAV_CROSSBAR_MM <= 0) ? 1u : 3u;
            to_stop(m, in, out, true);
            return;
        }
        float travel =
            enc_mean_travel(in->wheel_count, m->entry_origin) / ENC_COUNTS_PER_MM;
        /* Advance until the axle is over the crossing centre, then pivot. */
        float d = NAV_SENSOR_FORWARD_MM + NAV_CROSSBAR_MM * 0.5f - travel;
        /* Too late to stop at the crossing centre on the profile. */
        if (turning && (d < motion_stop_distance_mm(COURSE_SPEED_MMPS) ||
            d > MOTION_TARGET_MAX_MM))
        {
            out->nav_rejected = 2u;
            to_stop(m, in, out, true);
            return;
        }
        m->held = BARCODE_UNMAPPED;
        m->manual_held = 0u;
        m->in_broad = false;
        m->junction_locked = 1u;
        m->cross_side = m->line_side;
        m->exit_seen = 0u;
        mission_turn_target_set(m, action);
        m->turn_length_mm = turning ? (int16_t)lroundf(d) : 0;
        m->phase = JUNCTION_PHASE_ADVANCE;
        m->current_action = next_action_id(m);
        m->broad_origin[0] = in->wheel_count[0];
        m->broad_origin[1] = in->wheel_count[1];
        m->turn_sampled_ms = in->line_sampled_ms;
        enter(m, MISSION_NAVIGATE, in->now_ms);
        out->event_kind = MQTT_EVENT_TURN;
        out->event_arg = (uint8_t)action;
        handle_junction_turn(m, in, out);
        return;
    }
    if (broad)
    {
        /* Crossbar: no steering. Curve skew: keep the held correction, unless a
           queued turn may commit here and needs the heading steady (INT-153). */
        mark(m->dark_origin, in); /* the line is under both sensors */
        out->command = follow_command(in,
            m->in_broad && m->entry_curved && !held_turn ? m->line_steer_q : 0, 1u);
        return;
    }

    if (!in->usable_line || !in->guidance_valid)
    {
        to_stop(m, in, out, true);
        return;
    }
    int steer = line_digital_steering(m, in);
    /* Lost: white/white this far past the last touch. The drift makes a centred
       robot touch again well within it; before the first touch since entering
       FOLLOW the line is between the sensors and cannot leave unseen. */
    if (in->line_error == 0 && m->line_side != 0 &&
        travel_mm(in, m->dark_origin) >= LINE_LOST_MM)
    {
        if (!MISSION_RECOVERY_RELEASED)
        {
            /* Physical release is independent of the proposed search policy. */
            to_stop(m, in, out, true);
            out->line_lost = true;
            return;
        }
        enter(m, MISSION_REACQUIRE, in->now_ms);
        recovery_input_t initial =
        {
            .now_ms = in->now_ms,
                .count = {in->wheel_count[0], in->wheel_count[1]},
                .sampled_ms = in->line_sampled_ms};
        recovery_control_begin(&m->recovery, &initial);
        m->held = BARCODE_UNMAPPED;
        m->manual_held = 0u;
        m->in_broad = false;
        out->stop_scanner = true;
        out->scan_cancel = true;
        out->side_watch_on = false;
        return;
    }
    out->command = follow_command(in, steer, 1u);
}

/* ---- Bounded profile/coverage handover ---------------------------------- */
/* The range now: the last reading less the travel since it was sampled. */
static int32_t front_now_mm(const MissionInputs *in)
{
    float counts = ((float)enc_travel_signed(in->wheel_count[0], in->front_wheel_count[0]) +
        (float)enc_travel_signed(in->wheel_count[1], in->front_wheel_count[1])) * 0.5f;
    return (int32_t)in->front_mm - (int32_t)lroundf(counts / ENC_COUNTS_PER_MM);
}

static bool obstacle_in_stopping_range(const MissionInputs *in)
{
    return in->front_valid && in->front_age_ms <= BYPASS_FRONT_MAX_AGE_MS &&
        front_now_mm(in) <= (int32_t)MISSION_OBSTACLE_STOP_MM +
        (int32_t)motion_stop_distance_mm(COURSE_SPEED_MMPS);
}

static bool obstacle_ahead(const MissionInputs *in)
{
    return in->front_valid && in->front_age_ms <= BYPASS_FRONT_MAX_AGE_MS &&
        in->front_mm <= MISSION_OBSTACLE_STOP_MM;
}

static bypass_input_t bypass_input(const MissionInputs *in)
{
    return (bypass_input_t){
        .now_ms = in->now_ms,
            .count = {in->wheel_count[0], in->wheel_count[1]},
            .stationary = in->stationary,
            .cancel = in->stop_down,
            .functional = in->inputs_ok && !in->motion_stopped,
            .front_ms = in->front_sampled_ms,
            .front_sequence = in->front_sequence,
            .front_count = {in->front_wheel_count[0], in->front_wheel_count[1]},
            .front_mm = in->front_mm,
            .front_valid = in->front_valid,
            .side_ms = in->side_sampled_ms,
            .side_sequence = in->side_sequence,
            .side_mm = in->side_mm,
            .side_valid = in->side_valid,
            .line_ms = in->line_sampled_ms,
            .line_functional = in->usable_line,
            .line_black = in->left_black || in->right_black,
            .motion_status = in->motion_status,
            .motion_action = in->motion_action};
}

static void run_bypass(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (robot_time_elapsed(in->now_ms, m->bypass_started_ms) >=
        MISSION_BYPASS_TIMEOUT_MS)
    {
        to_stop(m, in, out, true);
        return;
    }
    bypass_input_t input = bypass_input(in);
    bypass_output_t result;
    bypass_control_step(&m->bypass, &input, &m->action_id, &result);
    out->command = result.command;
    out->side_watch_on = result.side_watch;
    out->side_watch_deg = result.side_angle_deg;
    if (result.depth_event)
    {
        out->event_kind = MQTT_EVENT_OBSTACLE;
        out->event_arg2 = result.depth_mm;
    }
    if (result.route == BYPASS_ROUTE_FAILED)
    {
        to_stop(m, in, out, true);
    }
    else if (result.route == BYPASS_ROUTE_DONE)
    {
        /* A sensor just touched the line, head-on: advance the axle onto it and
           pivot back along the original direction, as at a junction. */
        m->junction_locked = 1u;
        m->exit_seen = 0u;
        mission_turn_target_set(m, m->bypass.left_side ? BARCODE_LEFT : BARCODE_RIGHT);
        m->turn_length_mm = NAV_SENSOR_FORWARD_MM;
        m->phase = JUNCTION_PHASE_ADVANCE;
        m->current_action = next_action_id(m);
        m->broad_origin[0] = in->wheel_count[0];
        m->broad_origin[1] = in->wheel_count[1];
        m->turn_sampled_ms = in->line_sampled_ms;
        enter(m, MISSION_NAVIGATE, in->now_ms);
        handle_junction_turn(m, in, out);
    }
}

static void request_scan(Mission *m, const MissionInputs *in,
    MissionOutputs *out, bool second)
{
    m->scan_second = second;
    out->scan_keep_narrower = second;
    m->front_seen = (uint8_t)in->front_sequence;
    m->front_clear = 0u;
    m->scan_id = (uint8_t)(m->scan_id >= 15u ? 0u : m->scan_id + 1u);
    out->scan_request = true;
    out->scan_id = m->scan_id;
    m->scan_phase = 1u;
}

static void enter_bypass(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    enter(m, MISSION_SCAN, in->now_ms);
    m->scan_phase = 0u;
    m->scan_retries = 0u;
    m->scan_backing = 0u;
    m->scan_second = 0u;
    m->backed_mm = 0u;
    m->bypass_started_ms = in->now_ms;
    m->in_broad = false;
    /* Planned stop at MISSION_OBSTACLE_STOP_MM. Already inside it (an obstacle that
       appeared late): brake now rather than roll closer. */
    int32_t room = front_now_mm(in) - (int32_t)MISSION_OBSTACLE_STOP_MM;
    if (room <= 0)
    {
        m->scan_stopping = 0u;
        out->command = stop_command(in->now_ms);
        return;
    }
    m->scan_stopping = 1u;
    m->turn_length_mm = (int16_t)room;
    m->current_action = next_action_id(m);
    handle_scan(m, in, out);
}

/* True once the forward readings after a sweep settle it: one blocked reading,
   or MISSION_SCAN_CLEAR_SAMPLES clear ones in a row. The servo publishes no
   forward reading until it is back at 0, so these cannot be left over from the
   sweep. */
static bool front_checked(Mission *m, const MissionInputs *in)
{
    if ((uint8_t)in->front_sequence == m->front_seen)
    {
        return false;
    }
    m->front_seen = (uint8_t)in->front_sequence;
    if (in->front_valid && in->front_mm <=
        MISSION_OBSTACLE_STOP_MM + (uint32_t)m->backed_mm +
        MISSION_SCAN_CLEAR_MARGIN_MM)
    {
        m->front_clear = 0u;
        return true;
    }
    ++m->front_clear;
    return m->front_clear >= MISSION_SCAN_CLEAR_SAMPLES;
}

static void handle_scan(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (robot_time_elapsed(in->now_ms, m->bypass_started_ms) >=
        MISSION_BYPASS_TIMEOUT_MS)
    {
        to_stop(m, in, out, true);
        return;
    }
    if (m->scan_stopping)
    {
        bool ours = in->motion_action == m->current_action;
        if (ours && in->motion_status == RD_MOTION_FAILED)
        {
            to_stop(m, in, out, true);
        }
        else if (ours && in->motion_status == RD_MOTION_DONE)
        {
            m->scan_stopping = 0u;
        }
        else
        {
            /* Slow down on the profile; the rest of this file's stops are safety stops. */
            out->command = motion_command_make(MOTION_MODE_DISTANCE, MISSION_TRACK_SPEED_MMPS,
                m->turn_length_mm, MOTION_TURN_RAMPED, m->current_action, in->now_ms);
        }
        return;
    }
    if (m->scan_backing)
    {
        if (in->motion_action == m->current_action &&
            in->motion_status == RD_MOTION_FAILED)
        {
            to_stop(m, in, out, true);
        }
        else if (in->motion_action == m->current_action &&
            in->motion_status == RD_MOTION_DONE)
        {
            m->scan_backing = 0u;
            /* rescan: the old profile was from the closer position */
            m->scan_phase = 0u;
            m->scan_second = 0u;
            m->scan_retries = 0u;
        }
        else
        {
            out->command = action_command(MOTION_MODE_DISTANCE,
                BYPASS_SPEED_MMPS, -(float)m->backup_mm, m->current_action,
                in->now_ms);
        }
        return;
    }
    if (m->scan_phase == 0u)
    {
        if (in->stationary)
        {
            request_scan(m, in, out, false);
        }
        return;
    }
    if (in->scan_id != m->scan_id)
    {
        return; /* A previous scan result cannot satisfy a new request. */
    }
    if (in->scan_state == OBSTACLE_SCAN_FAILED ||
        (in->scan_state == OBSTACLE_SCAN_DONE && in->profile_valid))
    {
        if (!front_checked(m, in))
        {
            return;
        }
        if (m->front_clear >= MISSION_SCAN_CLEAR_SAMPLES)
        {
            /* the obstacle was removed */
            enter(m, MISSION_FOLLOW_LINE, in->now_ms);
            return;
        }
    }
    else
    {
        m->front_seen = (uint8_t)in->front_sequence; /* sweep still running */
    }
    /* A failed second sweep leaves the first sweep's profile in place. */
    bool use_first = in->scan_state == OBSTACLE_SCAN_FAILED &&
        m->scan_second && in->profile_valid;
    if (!use_first && (in->scan_state == OBSTACLE_SCAN_CANCELLED ||
        in->scan_state == OBSTACLE_SCAN_FAILED))
    {
        if (in->scan_state == OBSTACLE_SCAN_FAILED && in->stationary &&
            m->scan_retries < MISSION_SCAN_RETRIES)
        {
            ++m->scan_retries;
            request_scan(m, in, out, false);
        }
        else
        {
            to_stop(m, in, out, true);
        }
    }
    else if ((use_first || in->scan_state == OBSTACLE_SCAN_DONE) &&
        in->profile_valid)
    {
        ObstacleProfile profile =
        {
            .valid = true,
                .closest_mm = obstacle_profile_mm(in->profile_closest_mm),
                .width_mm = obstacle_profile_mm(in->profile_width_mm),
                .left_clearance_mm = obstacle_profile_mm(in->left_clearance_mm),
                .right_clearance_mm = obstacle_profile_mm(in->right_clearance_mm),
                .obstacle_forward_mm = in->profile_forward_mm,
                .obstacle_left_mm = in->profile_left_mm};
        bypass_input_t input = bypass_input(in);
        if (!MISSION_BYPASS_RELEASED)
        {
            to_stop(m, in, out, true);
        }
        else if (bypass_control_can_begin(&input, &profile,
            in->profile_planning_mm))
        {
            if (!m->scan_second)
            {
                request_scan(m, in, out, true);
            }
            else if (bypass_control_begin(&m->bypass, &input, &profile,
                in->profile_planning_mm, &m->action_id))
            {
                enter(m, MISSION_BYPASS, in->now_ms);
                run_bypass(m, in, out);
            }
            else
            {
                to_stop(m, in, out, true);
            }
        }
        else
        {
            /* Too close to pivot, or no room shown: reverse along the approach
               and scan again. */
            uint8_t backup = bypass_control_backup_mm(&profile,
                in->profile_planning_mm);
            backup = backup != 0u ? backup : BYPASS_NO_ROOM_STEP_MM;
            if ((unsigned)m->backed_mm + backup > BYPASS_BACKUP_MAX_MM)
            {
                to_stop(m, in, out, true);
                return;
            }
            m->scan_backing = 1u;
            m->backed_mm = (uint8_t)(m->backed_mm + backup);
            m->backup_mm = backup;
            m->current_action = next_action_id(m);
            out->command = action_command(MOTION_MODE_DISTANCE,
                BYPASS_SPEED_MMPS, -(float)backup, m->current_action,
                in->now_ms);
        }
    }
}

static void handle_junction_turn(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (!in->usable_line ||
        robot_time_elapsed(in->now_ms, in->line_sampled_ms) > IR_MAX_GAP_MS ||
        obstacle_ahead(in) ||
        robot_time_elapsed(in->now_ms, m->state_since_ms) >=
        MOTION_ACTION_TIMEOUT_MS ||
        m->phase > JUNCTION_PHASE_ADVANCE ||
        (in->motion_action == m->current_action &&
        in->motion_status == RD_MOTION_FAILED))
    {
        to_stop(m, in, out, true);
        return;
    }
    float heading = enc_heading_deg(
        enc_travel_signed(in->wheel_count[0], m->broad_origin[0]),
        enc_travel_signed(in->wheel_count[1], m->broad_origin[1]));
    bool broad = in->left_black && in->right_black;
    bool fresh = in->line_sampled_ms != m->turn_sampled_ms;
    if (fresh)
    {
        m->turn_sampled_ms = in->line_sampled_ms;
    }
    if (m->phase == JUNCTION_PHASE_ADVANCE && m->turn_code == 0u)
    {
        /* Straight crossing: roll over the bar at course speed with no steering,
           so neither sensor can steer on it, then track. */
        if (travel_mm(in, m->broad_origin) < NAV_STRAIGHT_MM)
        {
            out->command = follow_command(in, 0, 1u);
            return;
        }
        enter(m, MISSION_FOLLOW_LINE, in->now_ms);
        m->broad_origin[0] = in->wheel_count[0];
        m->broad_origin[1] = in->wheel_count[1];
        /* INT-139: keep the last side, so a white exit drifts back toward the line
           with line loss armed instead of driving on blind. */
        m->line_side = m->cross_side;
        m->line_crossed = 1u; /* the mark below is not a touch: no bend from it */
        mark(m->dark_origin, in);
        handle_follow(m, in, out);
        return;
    }
    if (m->phase == JUNCTION_PHASE_ADVANCE)
    {
        out->command = motion_command_make(MOTION_MODE_DISTANCE,
            MISSION_TRACK_SPEED_MMPS,
            m->turn_length_mm, MOTION_TURN_RAMPED,
            m->current_action, in->line_sampled_ms);
        if (in->motion_action == m->current_action &&
            in->motion_status == RD_MOTION_DONE)
        {
            m->phase = JUNCTION_PHASE_PAUSE_WAIT;
        }
        return;
    }
    if (m->phase == JUNCTION_PHASE_PAUSE_WAIT ||
        m->phase == JUNCTION_PHASE_BRAKE)
    {
        if (!in->stationary)
        {
            m->exit_seen = 0u;
            m->pause_since_ms = in->now_ms;
            return;
        }
        if (!m->exit_seen)
        {
            m->exit_seen = 1u;
            m->pause_since_ms = in->now_ms;
        }
        if (robot_time_elapsed(in->now_ms, m->pause_since_ms) < NAV_PAUSE_MS)
        {
            return;
        }
        m->exit_seen = 0u;
        if (m->phase == JUNCTION_PHASE_BRAKE)
        {
            enter(m, MISSION_FOLLOW_LINE, in->now_ms);
            m->broad_origin[0] = in->wheel_count[0];
            m->broad_origin[1] = in->wheel_count[1];
            /* INT-136: the pivot coasts past the branch, which can leave both
               sensors white with the line behind the trailing sensor. Lean back
               toward it, with line loss armed, instead of driving on blind. */
            m->line_side = m->turn_code == 1u ? 1 : -1;
            m->line_bend = 1u;
            mark(m->dark_origin, in);
            handle_follow(m, in, out);
            return;
        }
        m->phase = JUNCTION_PHASE_TURN;
        m->current_action = next_action_id(m);
        m->broad_origin[0] = in->wheel_count[0];
        m->broad_origin[1] = in->wheel_count[1];
        out->command = junction_turn_command(m, in);
        return;
    }
    if (m->phase != JUNCTION_PHASE_TURN)
    {
        to_stop(m, in, out, true);
        return;
    }
    float angle = heading * (mission_turn_target_get(m) < 0.0f ? -1.0f : 1.0f);
    float sense_from = m->turn_code == 3u ? NAV_UTURN_SENSE_DEG : NAV_TURN_SENSE_DEG;
    if (fresh && angle >= sense_from && !broad)
    {
        /* Stop once the leading sensor has crossed the new branch and left it, so
           the line lies between the sensors; stopping at first black left
           U-turns short in the reference repo. */
        bool leading_black = m->turn_code == 1u ? in->left_black : in->right_black;
        if (leading_black)
        {
            m->exit_seen = 1u;
        }
        else if (m->exit_seen)
        {
            m->phase = JUNCTION_PHASE_BRAKE; /* STOP until stationary, then follow */
            m->exit_seen = 0u;
            return;
        }
    }
    if (in->motion_action == m->current_action && in->motion_status == RD_MOTION_DONE)
    {
        to_stop(m, in, out, true); /* turned to the bound without finding the branch */
        return;
    }
    out->command = junction_turn_command(m, in);
}

static void handle_reacquire(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    recovery_input_t input =
    {
        .now_ms = in->now_ms,
            .count = {in->wheel_count[0], in->wheel_count[1]},
            .sampled_ms = in->line_sampled_ms,
            .functional = in->usable_line,
            .visible = in->line == IR_LINE_VISIBLE && in->guidance_valid,
            .black = in->left_black || in->right_black,
            .hazard = obstacle_ahead(in),
            .cancel = in->stop_down,
            .stationary = in->stationary,
            .motion_done = in->motion_action == m->current_action &&
            in->motion_status == RD_MOTION_DONE,
            .motion_failed = in->motion_action == m->current_action &&
            in->motion_status == RD_MOTION_FAILED};
    recovery_output_t result;
    recovery_control_step(&m->recovery, &input, &result);
    if (!MISSION_RECOVERY_RELEASED || result.phase == RECOVERY_FAILED)
    {
        to_stop(m, in, out, true);
        out->line_lost = true;
    }
    else if (result.phase == RECOVERY_DONE)
    {
        enter(m, MISSION_FOLLOW_LINE, in->now_ms);
        out->start_scanner = true;
    }
    else if (result.turn)
    {
        if (result.new_turn)
        {
            m->current_action = next_action_id(m);
        }
        out->command = action_command(MOTION_MODE_TURN, RECOVERY_SPEED_MMPS,
            result.turn_degrees, m->current_action,
            in->line_sampled_ms);
    }
}

bool mission_barcode_event_needed(const MissionInputs *in,
    const MissionOutputs *out)
{
    return in->have_barcode && out->event_kind != MQTT_EVENT_BARCODE &&
        in->barcode_letter < BARCODE_ALPHABET;
}

void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    *out = (MissionOutputs){.command = stop_command(in->now_ms)};

    note_onsets(m, in);
    bool moving = m->state != MISSION_READY && m->state != MISSION_STOPPED;
    if (m->state != MISSION_STOPPED &&
        (in->stop_down || in->safety_latched || !in->inputs_ok ||
        (moving && in->motion_stopped)))
    {
        /* Safety already called motion_stop() for its own causes. */
        to_stop(m, in, out, in->stop_down || !in->inputs_ok);
        return;
    }
    if ((m->state == MISSION_FOLLOW_LINE || m->state == MISSION_NAVIGATE) &&
        (!in->usable_line ||
        robot_time_elapsed(in->now_ms, in->line_sampled_ms) > IR_MAX_GAP_MS))
    {
        to_stop(m, in, out, true);
        return;
    }
    switch (m->state)
    {
    case MISSION_READY:
        handle_ready(m, in, out);
        break;
    case MISSION_FOLLOW_LINE:
        handle_follow(m, in, out);
        break;
    case MISSION_NAVIGATE:
        handle_junction_turn(m, in, out);
        break;
    case MISSION_SCAN:
        handle_scan(m, in, out);
        break;
    case MISSION_REACQUIRE:
        handle_reacquire(m, in, out);
        break;
    case MISSION_BYPASS:
        run_bypass(m, in, out);
        break;
    case MISSION_STOPPED:
        handle_stop(m, in, out);
        break;
    default:
        to_stop(m, in, out, true);
        break;
    }
    /* Every moving command on the course runs at the course
       speed; anything else is a defect, so stop rather than drive it. */
    if ((m->state == MISSION_FOLLOW_LINE || m->state == MISSION_NAVIGATE) &&
        motion_cmd_mode(&out->command) != MOTION_MODE_STOP &&
        motion_cmd_speed(&out->command) != (uint16_t)MISSION_TRACK_SPEED_MMPS)
    {
        to_stop(m, in, out, true);
    }
}

mqtt_line_trace_t mission_line_trace(const Mission *m, const MissionInputs *in,
    const MissionOutputs *out)
{
    bool follow = motion_cmd_mode(&out->command) == MOTION_MODE_FOLLOW;
    return (mqtt_line_trace_t){
        .sampled_ms = in->line_sampled_ms,
        .flags = (uint8_t)((in->left_black ? MQTT_LINE_LEFT_BLACK : 0u) |
            (in->right_black ? MQTT_LINE_RIGHT_BLACK : 0u) |
            (m->in_broad ? MQTT_LINE_IN_BROAD : 0u) |
            (m->junction_open ? MQTT_LINE_JUNCTION_OPEN : 0u) |
            (m->junction_qualified ? MQTT_LINE_JUNCTION_QUALIFIED : 0u) |
            (m->entry_curved ? MQTT_LINE_ENTRY_CURVED : 0u) |
            (m->junction_locked ? MQTT_LINE_JUNCTION_LOCKED : 0u) |
            (in->barcode_level ? MQTT_LINE_BARCODE_LEVEL : 0u)),
        .mission_state = m->state,
        .phase = m->phase,
        .wheel_count = {(uint16_t)in->wheel_count[0], (uint16_t)in->wheel_count[1]},
        .steer_q = m->line_steer_q,
        .law_q = (int8_t)(m->line_side * (m->line_bend ? 2 : 1)),
        .onset_skew_mm = in->left_black && in->right_black ?
            (uint8_t)fminf(onset_skew_mm(m), 254.0f) : 255u,
        .command_steer_q = follow ? motion_cmd_steering(&out->command) : 0,
        .command_mode = (uint8_t)motion_cmd_mode(&out->command),
        .command_speed = motion_cmd_speed(&out->command),
        .barcode_edges = in->barcode_edges};
}

bool mission_queue_direction(Mission *m, uint8_t action)
{
    if (m->state != MISSION_FOLLOW_LINE || m->in_broad || m->manual_held ||
        action < BARCODE_LEFT || action > BARCODE_U_TURN)
    {
        return false;
    }
    m->held = (barcode_action_t)action;
    m->manual_held = 1u;
    return true;
}
