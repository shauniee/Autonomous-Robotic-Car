#include "mission_logic.h"
#include <math.h>
#include "../common/robot_time_math.h"

#define LAST_ACTION_ID rd_mask(RD_MOTION_ACTION_BITS)

void mission_init(Mission *m, uint32_t now_ms)
{
    *m = (Mission){.state = MISSION_CALIBRATE, .state_since_ms = now_ms};
}

static void enter(Mission *m, MissionState next, uint32_t now_ms)
{
    m->state = next;
    m->state_since_ms = now_ms;
}

static uint8_t next_action_id(Mission *m)
{
    /* ID 1 is reserved for FOLLOW so a wrapped action ID can never alias it. */
    m->action_id = (uint8_t)(m->action_id >= LAST_ACTION_ID || m->action_id < 1u ? 2u : m->action_id + 1u);
    return m->action_id;
}

static MotionCommand stop_command(uint32_t now_ms)
{
    return (MotionCommand){.mode = MOTION_MODE_STOP, .sampled_ms = now_ms};
}

/* Starting an action: a fresh ID makes Motion begin it; the same ID continues it. */
static MotionCommand action_command(motion_mode_t mode, float speed, float target, uint8_t id, uint32_t now_ms)
{
    return (MotionCommand){.mode = mode, .speed_mmps = speed, .target = target, .action_id = id, .sampled_ms = now_ms};
}

static MotionCommand follow_command(const MissionInputs *in, float speed, float steering, uint8_t id)
{
    return (MotionCommand){.mode = MOTION_MODE_FOLLOW, .speed_mmps = speed, .steering = steering,
                           .action_id = id, .sampled_ms = in->line_sampled_ms};
}

static void to_stop(Mission *m, const MissionInputs *in, MissionOutputs *out, bool call_stop)
{
    enter(m, MISSION_STOP, in->now_ms);
    m->held = BARCODE_UNMAPPED;
    m->in_broad = false;
    m->start_pending = false;
    out->stop_now = call_stop;
    out->stop_scanner = true;
    out->command = stop_command(in->now_ms);
}

static void handle_calibrate(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    (void)out;
    if (in->calibration_valid)
    {
        enter(m, MISSION_READY, in->now_ms);
    }
}

static void handle_ready(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (m->start_pending)
    {
        if (!in->motion_stopped && !in->safety_latched && in->guidance_valid)
        {
            m->start_pending = false;
            m->direction = 0;
            m->visible_streak = 0u;
            m->in_broad = false;
            m->held = BARCODE_UNMAPPED;
            enter(m, MISSION_FOLLOW, in->now_ms);
            out->start_scanner = true;
        }
        else if (robot_time_elapsed(in->now_ms, m->state_since_ms) >= MISSION_START_WAIT_MS)
        {
            m->start_pending = false; /* Motion did not release: a new press is needed. */
        }
        return;
    }
    if (in->start_pressed && in->guidance_valid)
    {
        /* This press is the deliberate start: release the latch, then follow. */
        m->start_pending = true;
        m->state_since_ms = in->now_ms;
        out->request_reset = in->motion_stopped || in->safety_latched;
    }
}

static void handle_stop(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    /* A press releases the latch only; Mission stays idle in READY afterwards. */
    if (in->start_pressed && !in->stop_down)
    {
        out->request_reset = true;
    }
    if (!in->stop_down && !in->safety_latched && !in->motion_stopped && in->inputs_ok)
    {
        enter(m, in->calibration_valid ? MISSION_READY : MISSION_CALIBRATE, in->now_ms);
    }
}

static void handle_follow(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    if (in->have_barcode && in->barcode_action != BARCODE_UNMAPPED)
    {
        m->held = in->barcode_action; /* the newest read replaces an unused one */
    }

    bool broad = in->line == IR_LINE_BROAD_BLACK && in->usable_line;
    if (broad && !m->in_broad)
    {
        m->in_broad = true;
        m->broad_origin = in->travel_counts;
    }
    uint32_t broad_travel = m->in_broad ? enc_travel(in->travel_counts, m->broad_origin) : 0u;

    if (broad && broad_travel >= MISSION_FINISH_COUNTS)
    {
        enter(m, MISSION_COMPLETE, in->now_ms);
        out->stop_scanner = true;
        out->command = stop_command(in->now_ms);
        return;
    }
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
                m->phase = JUNCTION_PHASE_TURN;
                m->current_action = next_action_id(m);
                m->turn_target = action == BARCODE_LEFT ? 90.0f : (action == BARCODE_RIGHT ? -90.0f : 180.0f);
                enter(m, MISSION_JUNCTION_TURN, in->now_ms);
                out->command = action_command(MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, m->turn_target,
                                              m->current_action, in->now_ms);
                return;
            }
        }
    }

    if (m->recovering)
    {
        /* Bounded recovery: two fresh visible reads rejoin the line. */
        if (in->line == IR_LINE_VISIBLE && in->guidance_valid && in->usable_line)
        {
            if (++m->visible_streak >= 2u)
            {
                m->recovering = false;
            }
        }
        else
        {
            m->visible_streak = 0u;
        }
    }
    else if (in->line == IR_LINE_LOST)
    {
        if (m->direction == 0)
        {
            to_stop(m, in, out, true); /* never saw the line move: no side to search */
            return;
        }
        m->recovering = true;
        m->recovery_since_ms = in->now_ms;
        m->visible_streak = 0u;
        m->in_broad = false;
    }
    if (m->recovering)
    {
        if (robot_time_elapsed(in->now_ms, m->recovery_since_ms) >= RECOVERY_MS)
        {
            to_stop(m, in, out, true);
            return;
        }
        out->command = follow_command(in, MISSION_FOLLOW_SPEED_MMPS * 0.5f, m->direction > 0 ? 1.0f : -1.0f, 1u);
        return;
    }

    if (!in->usable_line || !in->guidance_valid)
    {
        to_stop(m, in, out, true);
        return;
    }
    float steer = in->steering;
    if (steer > MAX_STEER) steer = MAX_STEER;
    if (steer < -MAX_STEER) steer = -MAX_STEER;
    if (steer != 0.0f)
    {
        m->direction = steer > 0.0f ? 1 : -1;
    }
    bool is_broad = in->line == IR_LINE_BROAD_BLACK;
    out->command = follow_command(in, is_broad ? MISSION_BROAD_SPEED_MMPS : MISSION_FOLLOW_SPEED_MMPS,
                                  is_broad ? 0.0f : steer, 1u);
}

static void handle_junction_turn(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
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
            /* Next action starts only after DONE for the previous ID. */
            m->phase = JUNCTION_PHASE_CLEAR;
            m->current_action = next_action_id(m);
        }
        else
        {
            m->in_broad = false;
            m->recovering = false;
            m->visible_streak = 0u;
            enter(m, MISSION_FOLLOW, in->now_ms);
            out->command = stop_command(in->now_ms);
            return;
        }
    }
    /* Rewrite the current action each release; the ID keeps it the same action. */
    out->command = m->phase == JUNCTION_PHASE_TURN
        ? action_command(MOTION_MODE_TURN, MISSION_TURN_SPEED_MMPS, m->turn_target, m->current_action, in->now_ms)
        : action_command(MOTION_MODE_DISTANCE, MISSION_CLEAR_SPEED_MMPS, MISSION_CLEAR_MM, m->current_action, in->now_ms);
}

void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out)
{
    *out = (MissionOutputs){.command = stop_command(in->now_ms)};

    if (m->state != MISSION_STOP && (in->stop_down || in->safety_latched || !in->inputs_ok))
    {
        /* Safety already called s2_stop() for its own causes. */
        to_stop(m, in, out, in->stop_down || !in->inputs_ok);
        return;
    }
    switch (m->state)
    {
    case MISSION_CALIBRATE: handle_calibrate(m, in, out); break;
    case MISSION_READY: handle_ready(m, in, out); break;
    case MISSION_FOLLOW: handle_follow(m, in, out); break;
    case MISSION_JUNCTION_TURN: handle_junction_turn(m, in, out); break;
    case MISSION_COMPLETE:
        if (in->start_pressed)
        {
            enter(m, MISSION_READY, in->now_ms); /* a new press re-arms; nothing moves */
        }
        break;
    case MISSION_STOP: handle_stop(m, in, out); break;
    default: to_stop(m, in, out, true); break;
    }
}
