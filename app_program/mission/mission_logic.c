#include "mission_logic.h"
#include <math.h>
#include <stdlib.h>
#include "../common/robot_time_math.h"
#include "../subsystem2/encoder_math.h"

#define LAST_ACTION_ID rd_mask(RD_MOTION_ACTION_BITS)

static bool obstacle_ahead(const MissionInputs *in);
static void handle_reacquire(Mission *m, const MissionInputs *in,
    MissionOutputs *out);
static void enter_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out);

void mission_init(Mission *m, uint32_t now_ms)
{
    *m = (Mission){.state = MISSION_READY, .state_since_ms = now_ms};
}

static void line_pid_reset(Mission *m)
{
    m->line_integral_q = 0;
    m->line_pid_primed = false;
    m->line_steer_q = 0;
}

/* Private history: /32767 preserves exact +/-1 endpoints. Float is local
   computation only; clamp before narrowing and never store INT16_MIN. */
#define MISSION_PID_SCALE 32767.0f
_Static_assert(LINE_I_LIMIT <= 1.0f, "integral history scale requires limit <=1");
_Static_assert(MAX_STEER <= 1.0f, "held steering must fit signed Q7");

static int16_t mission_pid_encode(float value)
{
    if (!isfinite(value))
    {
        return 0;
    }
    value = fminf(fmaxf(value, -1.0f), 1.0f);
    return (int16_t)lroundf(value * MISSION_PID_SCALE);
}

static float mission_line_integral_get(const Mission *m)
{
    return (float)m->line_integral_q / MISSION_PID_SCALE;
}

static void mission_line_integral_set(Mission *m, float value)
{
    m->line_integral_q = mission_pid_encode(
        fminf(fmaxf(value, -LINE_I_LIMIT), LINE_I_LIMIT));
}

static float mission_line_prev_error_get(const Mission *m)
{
    return (float)m->line_prev_error_q / MISSION_PID_SCALE;
}

static void mission_line_prev_error_set(Mission *m, float value)
{
    m->line_prev_error_q = mission_pid_encode(value);
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
        return 180.0f;
    default:
        return 0.0f;
    }
}

static void mission_turn_target_set(Mission *m, barcode_action_t action)
{
    m->turn_code = action == BARCODE_LEFT ? 1u :
        (action == BARCODE_RIGHT ? 2u : (action == BARCODE_U_TURN ? 3u : 0u));
}

static void enter(Mission *m, MissionState next, uint32_t now_ms)
{
    m->state = next;
    m->state_since_ms = now_ms;
    m->line_loss_pending = 0u;
    line_pid_reset(m);	/* every run segment starts without PID history */
}

/* Line PID on the two-sensor error (right - left). It updates once per new IR
   acquisition (dt from acquisition times) and holds its output between
   samples. The nearly binary IR edge makes the error step, so it is first
   low-pass filtered (LINE_ERROR_TAU_MS); P, I and D all act on the filtered
   error. The steering may change by at most LINE_STEER_RATE per second, so a
   line found again after a gap is rejoined gently. The result is signed Q7
   steering limited to MAX_STEER. */
static int line_pid_steering(Mission *m, const MissionInputs *in)
{
    if (m->line_integral_q == INT16_MIN || m->line_prev_error_q == INT16_MIN)
    {
        line_pid_reset(m);
    }
    if (m->line_pid_primed && in->line_sampled_ms == m->line_pid_ms)
    {
        return m->line_steer_q;
    }
    float error = (float)in->line_error / (float)IR_SCALE_ONE;
    float integral = mission_line_integral_get(m);
    float derivative = 0.0f;
    float dt = (float)IR_SAMPLE_MS / 1000.0f;	/* nominal step for the first sample */
    uint32_t dt_ms = in->line_sampled_ms - m->line_pid_ms;
    if (m->line_pid_primed && dt_ms > 0u && dt_ms <= IR_MAX_GAP_MS)
    {
        dt = (float)dt_ms / 1000.0f;
        float prior_error = mission_line_prev_error_get(m);
        error = prior_error + (error - prior_error) * dt /
            (LINE_ERROR_TAU_MS / 1000.0f + dt);
        derivative = (error - prior_error) / dt;
        integral = fminf(fmaxf(integral + LINE_KI * error * dt,
            -LINE_I_LIMIT), LINE_I_LIMIT);
    }
    float steer = LINE_KP * error + integral + LINE_KD * derivative;
    float previous = (float)m->line_steer_q / (float)IR_SCALE_ONE;
    float step = LINE_STEER_RATE * dt;
    steer = fminf(fmaxf(steer, previous - step), previous + step);
    steer = fminf(fmaxf(steer, -MAX_STEER), MAX_STEER);
    mission_line_prev_error_set(m, error);
    mission_line_integral_set(m, integral);
    m->line_pid_ms = in->line_sampled_ms;
    m->line_pid_primed = true;
    m->line_steer_q = (int8_t)lroundf(steer * (float)IR_SCALE_ONE);
    return m->line_steer_q;
}

/* Follow speed slows with steering so a curve gets time to be corrected:
   full speed straight, (1 - LINE_CURVE_SLOWDOWN) of it at MAX_STEER. */
static float line_follow_speed(int steer_q)
{
    float share = fminf((float)abs(steer_q) / (MAX_STEER * (float)IR_SCALE_ONE), 1.0f);
    return floorf(MISSION_FOLLOW_SPEED_MMPS * (1.0f - LINE_CURVE_SLOWDOWN * share));
}

static uint8_t next_action_id(Mission *m)
{
    /* ID 1 is reserved for FOLLOW so a wrapped action ID can never alias it. */
    m->action_id = (uint8_t)(m->action_id >= LAST_ACTION_ID || m->action_id < 1u ? 2u : m->action_id + 1u);
    return m->action_id;
}

static MotionCommand stop_command(uint32_t now_ms)
{
    return motion_command_make(MOTION_MODE_STOP, 0.0f, 0.0f, 0, 0u, now_ms);
}

/* Action targets are integral mm or degrees; invalid/out-of-range values
   fail closed as a STOP command rather than being truncated. */
static MotionCommand action_command(motion_mode_t mode, float speed, float target, uint8_t id, uint32_t now_ms)
{
    return motion_command_make(mode, speed, target, 0, id, now_ms);
}

static MotionCommand follow_command(const MissionInputs *in, float speed, int steering_q, uint8_t id)
{
    if (in->hump_active)
    {
        speed *= 0.5f;
    }
    return motion_command_make(MOTION_MODE_FOLLOW, speed, 0.0f,
                               steering_q, id, in->line_sampled_ms);
}

/* Counts fresh visible line reads; true on the second in a row. */
static bool line_reacquired(Mission *m, const MissionInputs *in)
{
    if (in->line == IR_LINE_VISIBLE && in->guidance_valid && in->usable_line)
    {
        if (in->line_sampled_ms != m->confirmed_sample_ms)
        {
            m->confirmed_sample_ms = in->line_sampled_ms;
            if (m->visible_streak < MISSION_CONFIRM_SAMPLES)
            {
                ++m->visible_streak;
            }
        }
        return m->visible_streak >= MISSION_CONFIRM_SAMPLES;
    }
    m->visible_streak = 0u;
    return false;
}

static void to_stop(Mission *m, const MissionInputs *in, MissionOutputs *out, bool call_stop)
{
    enter(m, MISSION_STOPPED, in->now_ms);
    m->held = BARCODE_UNMAPPED;
    m->reacquire_bypass = 0u;
    m->in_broad = false;
    out->stop_now = call_stop;
    out->stop_scanner = true;
    out->scan_cancel = true;
    out->side_watch_on = false;
    out->command = stop_command(in->now_ms);
}

static void handle_ready(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (!in->start_pressed)
    {
        return;
    }
    if (!in->mapped || in->start_blocked || in->motion_stopped ||
        in->safety_latched || !in->guidance_valid || !in->calibration_valid)
    {
        out->start_refused = true;
        return;
    }
    m->visible_streak = 0u;
    m->in_broad = false;
    m->held = BARCODE_UNMAPPED;
    enter(m, MISSION_FOLLOW_LINE, in->now_ms);
    out->start_scanner = true;
}

static void handle_stop(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    (void)out;
    /* GP21 release is serviced by safety_step; GP20 never resets. */
    if (!in->stop_down && !in->safety_latched && !in->motion_stopped && in->inputs_ok)
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
    if (enc_travel(in->travel_counts, m->broad_origin) >= MISSION_JUNCTION_MIN_COUNTS)
    {
        m->junction_qualified = 1u;
    }
}

static void handle_follow(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (in->line != IR_LINE_LOST)
    {
        m->line_loss_pending = 0u;
    }
    if (in->have_barcode && in->barcode_action != BARCODE_UNMAPPED)
    {
        m->held = in->barcode_action; /* the newest read replaces an unused one */
        out->event_kind = MQTT_EVENT_BARCODE;
        out->event_arg = in->barcode_letter;
        out->event_arg2 = (uint16_t)in->barcode_action;
    }

    if (obstacle_ahead(in))
    {
        enter_bypass(m, in, out);
        return;
    }
    bool broad = in->line == IR_LINE_BROAD_BLACK && in->usable_line;
    if (broad && !m->in_broad)
    {
        m->in_broad = true;
        m->broad_origin = in->travel_counts;
        m->junction_open = 1u;
        m->junction_qualified = 0u;
        m->junction_sampled_ms = in->line_sampled_ms;
        m->broad_since_ms = in->now_ms;
        m->current_action = next_action_id(m);
        m->phase = JUNCTION_PHASE_CROSS;
        enter(m, MISSION_NAVIGATE, in->now_ms);
        /* Include Motion's completion tolerance so DONE cannot occur before
           the requested minimum travel under the configured encoder model. */
        out->command = action_command(MOTION_MODE_DISTANCE,
            MISSION_BROAD_SPEED_MMPS,
            MISSION_BROAD_FORWARD_MM + MOTION_DIST_TOL_MM,
            m->current_action, in->line_sampled_ms);
        return;
    }
    observe_junction(m, in);

    if (m->in_broad && !broad && in->usable_line)
    {
        m->in_broad = false;
        if (m->junction_qualified)
        {
            /* Cross junction: execute the held command, else carry straight on. */
            barcode_action_t action = m->held;
            m->held = BARCODE_UNMAPPED;
            if (action == BARCODE_LEFT || action == BARCODE_RIGHT || action == BARCODE_U_TURN)
            {
                m->phase = action == BARCODE_U_TURN ?
                    JUNCTION_PHASE_PAUSE_WAIT : JUNCTION_PHASE_TURN;
                m->current_action = next_action_id(m);
                mission_turn_target_set(m, action);
                enter(m, MISSION_NAVIGATE, in->now_ms);
                out->event_kind = MQTT_EVENT_TURN;
                out->event_arg = (uint8_t)action;
                if (m->phase == JUNCTION_PHASE_TURN)
                {
                    out->command = action_command(MOTION_MODE_TURN,
                        MISSION_TURN_SPEED_MMPS, mission_turn_target_get(m),
                        m->current_action, in->now_ms);
                }
                return;
            }
            /* Straight and unmapped crossings still confirm the outgoing
               branch using distinct acquisitions before ordinary following. */
            m->phase = JUNCTION_PHASE_CONFIRM;
            m->visible_streak = 0u;
            m->confirmed_sample_ms = in->line_sampled_ms;
            enter(m, MISSION_NAVIGATE, in->now_ms);
            return;
        }
    }

    if (broad)
    {
        /* The minimum distance is complete, but the intersection may be
           wider. Continue straight until black clears, with a total bound. */
        if (robot_time_elapsed(in->now_ms, m->broad_since_ms) >=
            MOTION_ACTION_TIMEOUT_MS)
        {
            to_stop(m, in, out, true);
            return;
        }
        m->visible_streak = 0u;
        line_pid_reset(m);
        out->command = follow_command(in, MISSION_BROAD_SPEED_MMPS, 0, 1u);
        return;
    }

    if (in->line == IR_LINE_LOST)
    {
        /* White/white is usable evidence; failed or old acquisition is not. */
        if (!in->usable_line ||
            robot_time_elapsed(in->now_ms, in->line_sampled_ms) > IR_MAX_GAP_MS)
        {
            to_stop(m, in, out, true);
            return;
        }
        if (!m->line_loss_pending ||
            robot_time_elapsed(in->line_sampled_ms,
                m->line_lost_sampled_ms) > IR_MAX_GAP_MS)
        {
            m->line_loss_pending = 1u;
            m->line_lost_since_ms = in->line_sampled_ms;
        }
        m->line_lost_sampled_ms = in->line_sampled_ms;
        if (robot_time_elapsed(in->line_sampled_ms,
                m->line_lost_since_ms) <= MISSION_LINE_LOST_MS)
        {
            /* Keep the last tracking correction through a brief white gap.
               Renew freshness from IR, never from this loop's wall clock. */
            out->command = follow_command(in,
                line_follow_speed(m->line_steer_q), m->line_steer_q, 1u);
            return;
        }
        if (!MISSION_RECOVERY_RELEASED)
        {
            /* Physical release is independent of the proposed search policy. */
            to_stop(m, in, out, true);
            out->line_lost = true;
            return;
        }
        enter(m, MISSION_REACQUIRE, in->now_ms);
        recovery_input_t initial = {.now_ms = in->now_ms,
            .count = {in->wheel_count[0], in->wheel_count[1]},
            .sampled_ms = in->line_sampled_ms};
        recovery_control_begin(&m->recovery, &initial);
        m->visible_streak = 0u;
        m->confirmed_sample_ms = in->line_sampled_ms;
        m->in_broad = false;
        handle_reacquire(m, in, out);
        return;
    }

    if (!in->usable_line || !in->guidance_valid)
    {
        to_stop(m, in, out, true);
        return;
    }
    int steer = line_pid_steering(m, in);
    out->command = follow_command(in, line_follow_speed(steer), steer, 1u);
}


static void handle_crossing(Mission *m, const MissionInputs *in,
                            MissionOutputs *out)
{
    observe_junction(m, in);
    if (!in->usable_line || obstacle_ahead(in) ||
        robot_time_elapsed(in->now_ms, m->broad_since_ms) >=
            MOTION_ACTION_TIMEOUT_MS ||
        (in->motion_action == m->current_action &&
         in->motion_status == RD_MOTION_FAILED))
    {
        to_stop(m, in, out, true);
        return;
    }
    if (in->motion_action == m->current_action &&
        in->motion_status == RD_MOTION_DONE)
    {
        enter(m, MISSION_FOLLOW_LINE, in->now_ms);
        handle_follow(m, in, out);
        return;
    }
    /* Retain the action ID and original acquisition time on each renewal.
       Leaving black early must not shorten the requested minimum travel. */
    out->command = action_command(MOTION_MODE_DISTANCE,
        MISSION_BROAD_SPEED_MMPS,
        MISSION_BROAD_FORWARD_MM + MOTION_DIST_TOL_MM,
        m->current_action, in->line_sampled_ms);
}


/* ---- Bounded profile/coverage handover ---------------------------------- */
static bool obstacle_ahead(const MissionInputs *in)
{
    return in->front_valid && in->front_age_ms <= BYPASS_FRONT_MAX_AGE_MS &&
        in->front_mm <= MISSION_OBSTACLE_STOP_MM;
}

static bypass_input_t bypass_input(Mission *m, const MissionInputs *in)
{
    return (bypass_input_t){.now_ms = in->now_ms,
        .count = {in->wheel_count[0], in->wheel_count[1]},
        .stationary = in->stationary, .cancel = in->stop_down,
        .functional = in->inputs_ok && !in->motion_stopped,
        .front_ms = in->front_sampled_ms, .front_sequence = in->front_sequence,
        .front_count = in->front_travel_counts, .front_mm = in->front_mm,
        .front_valid = in->front_valid, .side_ms = in->side_sampled_ms,
        .side_sequence = in->side_sequence, .side_mm = in->side_mm,
        .side_valid = in->side_valid, .line_ms = in->line_sampled_ms,
        .line_functional = in->usable_line,
        .line_visible = in->line == IR_LINE_VISIBLE && in->guidance_valid,
        .line_error_q = in->line_error,
        .line_steering_q = (int8_t)line_pid_steering(m, in),
        .motion_status = in->motion_status, .motion_action = in->motion_action};
}

static void run_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (robot_time_elapsed(in->now_ms, m->bypass_started_ms) >= MISSION_BYPASS_TIMEOUT_MS)
    {
        to_stop(m, in, out, true);
        return;
    }
    bypass_input_t input = bypass_input(m, in);
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
        m->reacquire_bypass = 0u;
        enter(m, MISSION_FOLLOW_LINE, in->now_ms);
    }
    else if (result.route == BYPASS_ROUTE_LINE && m->state != MISSION_REACQUIRE)
    {
        m->reacquire_bypass = 1u;
        enter(m, MISSION_REACQUIRE, in->now_ms);
    }
}

static void request_scan(Mission *m, MissionOutputs *out)
{
    m->scan_id = (uint8_t)(m->scan_id >= 15u ? 0u : m->scan_id + 1u);
    out->scan_request = true;
    out->scan_id = m->scan_id;
    m->scan_phase = 1u;
}

static void enter_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    enter(m, MISSION_SCAN, in->now_ms);
    m->scan_phase = 0u;
    m->scan_retries = 0u;
    m->bypass_started_ms = in->now_ms;
    m->in_broad = false;
    out->command = stop_command(in->now_ms);
}

static void handle_scan(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (robot_time_elapsed(in->now_ms, m->bypass_started_ms) >= MISSION_BYPASS_TIMEOUT_MS)
    {
        to_stop(m, in, out, true);
        return;
    }
    if (m->scan_phase == 0u)
    {
        if (in->stationary)
        {
            request_scan(m, out);
        }
        return;
    }
    if (in->scan_id != m->scan_id)
    {
        return; /* A previous scan result cannot satisfy a new request. */
    }
    if (in->scan_state == S5_SCAN_CANCELLED || in->scan_state == S5_SCAN_FAILED)
    {
        if (in->scan_state == S5_SCAN_FAILED && in->stationary &&
            m->scan_retries < MISSION_SCAN_RETRIES)
        {
            ++m->scan_retries;
            request_scan(m, out);
        }
        else
        {
            to_stop(m, in, out, true);
        }
    }
    else if (in->scan_state == S5_SCAN_DONE && in->profile_valid)
    {
        ObstacleProfile profile = {.valid = true, .closest_mm = in->profile_closest_mm,
            .left_clearance_mm = (uint16_t)in->left_clearance_mm,
            .right_clearance_mm = (uint16_t)in->right_clearance_mm,
            .obstacle_forward_mm = in->profile_forward_mm,
            .obstacle_left_mm = in->profile_left_mm};
        bypass_input_t input = bypass_input(m, in);
        if (!MISSION_BYPASS_RELEASED || !bypass_control_begin(&m->bypass, &input,
            &profile, in->profile_planning_mm, &m->action_id))
        {
            to_stop(m, in, out, true);
        }
        else
        {
            enter(m, MISSION_BYPASS, in->now_ms);
            run_bypass(m, in, out);
        }
    }
}

static void handle_junction_turn(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (m->phase > JUNCTION_PHASE_CONFIRM || obstacle_ahead(in) ||
        robot_time_elapsed(in->now_ms, m->state_since_ms) >=
            MOTION_ACTION_TIMEOUT_MS)
    {
        to_stop(m, in, out, true);
        return;
    }
    if (m->phase == JUNCTION_PHASE_PAUSE_WAIT)
    {
        if (in->stationary)
        {
            m->pause_since_ms = in->now_ms;
            m->phase = JUNCTION_PHASE_PAUSE;
        }
        return;
    }
    if (m->phase == JUNCTION_PHASE_PAUSE)
    {
        if (!in->stationary)
        {
            m->phase = JUNCTION_PHASE_PAUSE_WAIT;
            return;
        }
        else if (robot_time_elapsed(in->now_ms, m->pause_since_ms) >=
            MISSION_UTURN_PAUSE_MS)
        {
            m->phase = JUNCTION_PHASE_TURN;
        }
        else
        {
            return;
        }
    }
    if (m->phase == JUNCTION_PHASE_CONFIRM)
    {
        if (line_reacquired(m, in))
        {
            m->in_broad = false;
            enter(m, MISSION_FOLLOW_LINE, in->now_ms);
            return;
        }
        if (!in->usable_line)
        {
            to_stop(m, in, out, true);
            return;
        }
        out->command = follow_command(in, MISSION_BROAD_SPEED_MMPS, 0, 1u);
        return;
    }
    bool mine = in->motion_action == m->current_action;
    if (mine && in->motion_status == RD_MOTION_FAILED)
    {
        to_stop(m, in, out, true);
        return;
    }
    if (mine && in->motion_status == RD_MOTION_DONE)
    {
        if (m->phase == JUNCTION_PHASE_TURN)
        {
            m->phase = JUNCTION_PHASE_CLEAR;
            m->current_action = next_action_id(m);
        }
        else
        {
            m->phase = JUNCTION_PHASE_CONFIRM;
            m->visible_streak = 0u;
            m->confirmed_sample_ms = in->line_sampled_ms;
            return;
        }
    }
    out->command = m->phase == JUNCTION_PHASE_TURN
        ? action_command(MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS,
            mission_turn_target_get(m), m->current_action, in->now_ms)
        : action_command(MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS,
            MISSION_CLEAR_MM, m->current_action, in->now_ms);
}

static void handle_reacquire(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    recovery_input_t input = {.now_ms = in->now_ms,
        .count = {in->wheel_count[0], in->wheel_count[1]},
        .sampled_ms = in->line_sampled_ms, .functional = in->usable_line,
        .visible = in->line == IR_LINE_VISIBLE && in->guidance_valid,
        .hazard = obstacle_ahead(in), .cancel = in->stop_down,
        .error_q = in->line_error,
        .steering_q = (int8_t)line_pid_steering(m, in)};
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
    }
    else
    {
        out->command = follow_command(in, RECOVERY_SPEED_MMPS, result.steering_q, 1u);
    }
}

void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    *out = (MissionOutputs){.command = stop_command(in->now_ms)};

    bool moving = m->state != MISSION_READY && m->state != MISSION_STOPPED;
    if (m->state != MISSION_STOPPED && (in->stop_down || in->safety_latched ||
        !in->inputs_ok || (moving && in->motion_stopped)))
    {
        /* Safety already called s2_stop() for its own causes. */
        to_stop(m, in, out, in->stop_down || !in->inputs_ok);
        return;
    }
    switch (m->state)
    {
    case MISSION_READY: handle_ready(m, in, out); break;
    case MISSION_FOLLOW_LINE: handle_follow(m, in, out); break;
    case MISSION_NAVIGATE:
        if (m->phase == JUNCTION_PHASE_CROSS)
        {
            handle_crossing(m, in, out);
        }
        else
        {
            handle_junction_turn(m, in, out);
        }
        break;
    case MISSION_SCAN: handle_scan(m, in, out); break;
    case MISSION_REACQUIRE:
        if (m->reacquire_bypass)
        {
            run_bypass(m, in, out);
        }
        else
        {
            handle_reacquire(m, in, out);
        }
        break;
    case MISSION_BYPASS: run_bypass(m, in, out); break;
    case MISSION_STOPPED: handle_stop(m, in, out); break;
    default: to_stop(m, in, out, true); break;
    }
}
