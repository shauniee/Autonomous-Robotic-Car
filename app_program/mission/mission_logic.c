#include "mission_logic.h"
#include <math.h>
#include <stdlib.h>
#include "../common/robot_time_math.h"
#include "../subsystem2/encoder_math.h"

#define LAST_ACTION_ID rd_mask(RD_MOTION_ACTION_BITS)

static bool obstacle_ahead(const MissionInputs *in);
static void enter_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out);

void mission_init(Mission *m, uint32_t now_ms)
{
    *m = (Mission){.state = MISSION_READY, .state_since_ms = now_ms};
}

static void line_pid_reset(Mission *m)
{
    m->line_integral = 0.0f;
    m->line_pid_primed = false;
    m->line_steer_q = 0;
}

static void enter(Mission *m, MissionState next, uint32_t now_ms)
{
    m->state = next;
    m->state_since_ms = now_ms;
    line_pid_reset(m);	/* every run segment starts without PID history */
    if (next == MISSION_FOLLOW_LINE)
    {
        m->line_seen_ms = now_ms;	/* following only starts on a visible line */
    }
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
    if (m->line_pid_primed && in->line_sampled_ms == m->line_pid_ms)
    {
        return m->line_steer_q;
    }
    float error = (float)in->line_error / (float)IR_SCALE_ONE;
    float derivative = 0.0f;
    float dt = (float)IR_SAMPLE_MS / 1000.0f;	/* nominal step for the first sample */
    uint32_t dt_ms = in->line_sampled_ms - m->line_pid_ms;
    if (m->line_pid_primed && dt_ms > 0u && dt_ms <= IR_MAX_GAP_MS)
    {
        dt = (float)dt_ms / 1000.0f;
        error = m->line_prev_error + (error - m->line_prev_error) * dt / (LINE_ERROR_TAU_MS / 1000.0f + dt);
        derivative = (error - m->line_prev_error) / dt;
        m->line_integral = fminf(fmaxf(m->line_integral + LINE_KI * error * dt, -LINE_I_LIMIT), LINE_I_LIMIT);
    }
    float steer = LINE_KP * error + m->line_integral + LINE_KD * derivative;
    float previous = (float)m->line_steer_q / (float)IR_SCALE_ONE;
    float step = LINE_STEER_RATE * dt;
    steer = fminf(fmaxf(steer, previous - step), previous + step);
    steer = fminf(fmaxf(steer, -MAX_STEER), MAX_STEER);
    m->line_prev_error = error;
    m->line_pid_ms = in->line_sampled_ms;
    m->line_pid_primed = true;
    m->line_steer_q = (int)lroundf(steer * (float)IR_SCALE_ONE);
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
            if (m->visible_streak < 2u)
            {
                ++m->visible_streak;
            }
        }
        return m->visible_streak >= 2u;
    }
    m->visible_streak = 0u;
    return false;
}

static void to_stop(Mission *m, const MissionInputs *in, MissionOutputs *out, bool call_stop)
{
    enter(m, MISSION_STOPPED, in->now_ms);
    m->held = BARCODE_UNMAPPED;
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
    m->direction = 0;
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

static void handle_follow(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
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
        m->broad_since_ms = in->now_ms;
        m->current_action = next_action_id(m);
        m->recovering = false;
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
    uint32_t broad_travel = m->in_broad ? enc_travel(in->travel_counts, m->broad_origin) : 0u;

    if (m->in_broad && !broad && in->usable_line)
    {
        m->in_broad = false;
        if (broad_travel >= MISSION_JUNCTION_MIN_COUNTS)
        {
            /* Cross junction: execute the held command, else carry straight on. */
            barcode_action_t action = m->held;
            m->held = BARCODE_UNMAPPED;
            if (action == BARCODE_LEFT || action == BARCODE_RIGHT || action == BARCODE_U_TURN)
            {
                m->phase = action == BARCODE_U_TURN ?
                    JUNCTION_PHASE_PAUSE_WAIT : JUNCTION_PHASE_TURN;
                m->current_action = next_action_id(m);
                m->turn_target = action == BARCODE_LEFT ? 90.0f : (action == BARCODE_RIGHT ? -90.0f : 180.0f);
                enter(m, MISSION_NAVIGATE, in->now_ms);
                out->event_kind = MQTT_EVENT_TURN;
                out->event_arg = (uint8_t)action;
                if (m->phase == JUNCTION_PHASE_TURN)
                {
                    out->command = action_command(MOTION_MODE_TURN,
                        MISSION_TURN_SPEED_MMPS, m->turn_target,
                        m->current_action, in->now_ms);
                }
                return;
            }
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
        m->recovering = false;
        m->visible_streak = 0u;
        line_pid_reset(m);
        out->command = follow_command(in, MISSION_BROAD_SPEED_MMPS, 0, 1u);
        return;
    }

    if (in->line == IR_LINE_VISIBLE && in->guidance_valid)
    {
        m->line_seen_ms = in->now_ms;
    }
    if (!m->recovering && in->line == IR_LINE_LOST && in->usable_line &&
        robot_time_elapsed(in->now_ms, m->line_seen_ms) < LINE_CENTRE_GAP_MS)
    {
        /* Both sensors white just after seeing the line: with the sensors on the
           line's edges the line is between them, i.e. centred. Keep both wheels
           driving under the PID (the error reads about zero) instead of searching. */
        int steer = line_pid_steering(m, in);
        out->command = follow_command(in, line_follow_speed(steer), steer, 1u);
        return;
    }
    if (m->recovering)
    {
        /* Bounded recovery: two fresh visible reads rejoin the line. */
        if (line_reacquired(m, in))
        {
            m->recovering = false;
            enter(m, MISSION_FOLLOW_LINE, in->now_ms);
            line_pid_reset(m); /* no derivative across the search gap */
        }
    }
    else if (in->line == IR_LINE_LOST)
    {
        if (m->direction == 0 && APP_NO_GUARDS)
        {
            m->direction = 1;	/* bench build: search one side rather than stop */
        }
        if (m->direction == 0)
        {
            to_stop(m, in, out, true); /* never saw the line move: no side to search */
            out->line_lost = true;
            return;
        }
        m->recovering = true;
        enter(m, MISSION_REACQUIRE, in->now_ms);
        line_pid_reset(m);
        m->recovery_since_ms = in->now_ms;
        m->visible_streak = 0u;
        m->in_broad = false;
    }
    if (m->recovering)
    {
        if (!APP_NO_GUARDS && robot_time_elapsed(in->now_ms, m->recovery_since_ms) >= RECOVERY_MS)
        {
            to_stop(m, in, out, true);
            out->line_lost = true;
            return;
        }
        /* Search arc at MAX_STEER: both wheels keep turning, toward the last side. */
        int search_q = (int)lroundf(MAX_STEER * (float)IR_SCALE_ONE);
        out->command = follow_command(in, MISSION_FOLLOW_SPEED_MMPS * 0.5f, m->direction > 0 ? search_q : -search_q, 1u);
        return;
    }

    if (!in->usable_line || !in->guidance_valid)
    {
        to_stop(m, in, out, true);
        return;
    }
    int steer = line_pid_steering(m, in);
    if (steer != 0) m->direction = steer > 0 ? 1 : -1;
    out->command = follow_command(in, line_follow_speed(steer), steer, 1u);
}


static void handle_crossing(Mission *m, const MissionInputs *in,
                            MissionOutputs *out)
{
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


/* ---- Bypass ------------------------------------------------------------ */

static void begin_action(Mission *m, MissionBypassStep step, motion_mode_t mode, float speed, float target)
{
    m->bypass_step = step;
    m->current_action = next_action_id(m);
    m->action_mode = mode;
    m->action_speed = speed;
    m->action_target = target;
}

static bool action_done(const Mission *m, const MissionInputs *in)
{
    return in->motion_action == m->current_action && in->motion_status == RD_MOTION_DONE;
}

static bool obstacle_ahead(const MissionInputs *in)
{
    return in->front_valid && in->front_age_ms <= 3u * SONAR_REPEAT_MS && in->front_mm <= MISSION_OBSTACLE_STOP_MM;
}

static void enter_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    enter(m, MISSION_SCAN, in->now_ms);
    m->bypass_step = BYPASS_STOPPING;
    m->bypass_started_ms = in->now_ms;
    m->in_broad = false;
    out->command = stop_command(in->now_ms);
}

static void handle_bypass(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (robot_time_elapsed(in->now_ms, m->bypass_started_ms) >= MISSION_BYPASS_TIMEOUT_MS)
    {
        out->scan_cancel = true;
        out->side_watch_on = false;
        to_stop(m, in, out, true);
        return;
    }
    bool moving_forward = m->bypass_step == BYPASS_OFFSET_OUT || m->bypass_step == BYPASS_OFFSET_IN;
    bool failed = m->current_action == in->motion_action && in->motion_status == RD_MOTION_FAILED &&
                  m->bypass_step > BYPASS_SCANNING && m->bypass_step != BYPASS_REACQUIRE;
    if (failed || (moving_forward && obstacle_ahead(in)))
    {
        /* A second obstacle (or any failed leg) stops the robot. */
        out->side_watch_on = false;
        to_stop(m, in, out, true);
        return;
    }

    switch (m->bypass_step)
    {
    case BYPASS_STOPPING:
        if (in->stationary)
        {
            out->scan_request = true;
            m->bypass_step = BYPASS_SCANNING;
        }
        break;
    case BYPASS_SCANNING:
        if (in->scan_state == S5_SCAN_FAILED || in->scan_state == S5_SCAN_CANCELLED)
        {
            to_stop(m, in, out, true);
        }
        else if (in->scan_state == S5_SCAN_DONE && in->profile_valid)
        {
            m->bypass_side = in->left_clearance_mm >= in->right_clearance_mm ? 1 : -1;
            uint32_t clearance = m->bypass_side > 0 ? in->left_clearance_mm : in->right_clearance_mm;
            if (clearance < MISSION_BYPASS_MIN_CLEAR_MM)
            {
                to_stop(m, in, out, true); /* neither side is wide enough */
            }
            else
            {
                enter(m, MISSION_BYPASS, in->now_ms);
                begin_action(m, BYPASS_TURN_OUT, MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, (float)m->bypass_side * 90.0f);
            }
        }
        break;
    case BYPASS_TURN_OUT:
        if (action_done(m, in))
            begin_action(m, BYPASS_OFFSET_OUT, MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS, MISSION_BYPASS_OFFSET_MM);
        break;
    case BYPASS_OFFSET_OUT:
        if (action_done(m, in))
            begin_action(m, BYPASS_TURN_PARALLEL, MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, (float)-m->bypass_side * 90.0f);
        break;
    case BYPASS_TURN_PARALLEL:
        if (action_done(m, in))
        {
            m->pass_baseline_set = false;
            m->bypass_origin = in->travel_counts;
            begin_action(m, BYPASS_PASS, MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS, MISSION_PASS_LEG_MM);
        }
        break;
    case BYPASS_PASS:
        /* Servo held toward the obstacle; the first valid range is the baseline,
           a jump (or loss of echo) afterwards is the far end of the obstacle. */
        out->side_watch_on = true;
        out->side_watch_deg = (int16_t)(-m->bypass_side * SERVO_MAX_DEG);
        if (!m->pass_baseline_set)
        {
            if (in->side_valid)
            {
                m->pass_baseline_set = true;
                m->pass_baseline_mm = in->side_mm;
            }
        }
        else if (in->side_valid && in->side_mm > m->pass_baseline_mm + MISSION_PASS_JUMP_MM)
        {
            float depth_mm = (float)enc_travel(in->travel_counts, m->bypass_origin) / ENC_COUNTS_PER_MM;
            out->event_kind = MQTT_EVENT_OBSTACLE;
            out->event_arg2 = (uint16_t)(depth_mm >= 65535.0f ? 65535u : depth_mm + 0.5f);
            begin_action(m, BYPASS_PASS_MARGIN, MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS,
                         S5_BODY_LENGTH_MM + S5_CLEARANCE_MM);
        }
        else if (action_done(m, in))
        {
            to_stop(m, in, out, true); /* leg ended without ever passing the obstacle */
            out->side_watch_on = false;
        }
        break;
    case BYPASS_PASS_MARGIN:
        if (action_done(m, in))
            begin_action(m, BYPASS_TURN_IN, MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, (float)-m->bypass_side * 90.0f);
        break;
    case BYPASS_TURN_IN:
        if (action_done(m, in))
            begin_action(m, BYPASS_OFFSET_IN, MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS, MISSION_BYPASS_OFFSET_MM);
        break;
    case BYPASS_OFFSET_IN:
        if (action_done(m, in))
            begin_action(m, BYPASS_TURN_FORWARD, MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, (float)m->bypass_side * 90.0f);
        break;
    case BYPASS_TURN_FORWARD:
        if (action_done(m, in))
        {
            m->bypass_step = BYPASS_REACQUIRE;
            m->visible_streak = 0u;
        }
        break;
    case BYPASS_REACQUIRE:
        /* Creep straight on until two fresh visible reads rejoin the original line. */
        if (line_reacquired(m, in))
        {
            m->direction = 0;
            m->recovering = false;
            enter(m, MISSION_FOLLOW_LINE, in->now_ms);
            out->command = stop_command(in->now_ms);
            return;
        }
        out->command = follow_command(in, MISSION_BROAD_SPEED_MMPS * 0.5f, 0.0f, 1u);
        return;
    default:
        to_stop(m, in, out, true);
        return;
    }
    if (m->state == MISSION_BYPASS && m->bypass_step >= BYPASS_TURN_OUT && m->bypass_step != BYPASS_REACQUIRE)
    {
        out->command = action_command(m->action_mode, m->action_speed, m->action_target, m->current_action, in->now_ms);
    }
}

static void handle_junction_turn(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (obstacle_ahead(in) ||
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
            m->recovering = false;
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
            m->turn_target, m->current_action, in->now_ms)
        : action_command(MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS,
            MISSION_CLEAR_MM, m->current_action, in->now_ms);
}

static void handle_reacquire(Mission *m, const MissionInputs *in,
    MissionOutputs *out)
{
    if (!in->usable_line || obstacle_ahead(in) ||
        robot_time_elapsed(in->now_ms, m->recovery_since_ms) >= RECOVERY_MS)
    {
        to_stop(m, in, out, true);
        out->line_lost = true;
        return;
    }
    if (line_reacquired(m, in))
    {
        m->recovering = false;
        enter(m, MISSION_FOLLOW_LINE, in->now_ms);
        return;
    }
    /* Existing bounded arc retained provisionally. The requested moving
       sweep's angles/travel cap remain D07; no unmeasured sweep is enabled. */
    int search_q = (int)lroundf(MAX_STEER * (float)IR_SCALE_ONE);
    out->command = follow_command(in, MISSION_FOLLOW_SPEED_MMPS * 0.5f,
        m->direction > 0 ? search_q : -search_q, 1u);
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
    case MISSION_SCAN: handle_bypass(m, in, out); break;
    case MISSION_REACQUIRE: handle_reacquire(m, in, out); break;
    case MISSION_BYPASS: handle_bypass(m, in, out); break;
    case MISSION_STOPPED: handle_stop(m, in, out); break;
    default: to_stop(m, in, out, true); break;
    }
}
