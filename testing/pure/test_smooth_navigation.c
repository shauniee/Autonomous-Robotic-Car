#include "../../app_program/mission/mission_logic.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static MissionInputs input(void)
{
    return (MissionInputs){.now_ms = 100,
            .line_sampled_ms = 100,
            .mapped = true,
            .inputs_ok = true,
            .usable_line = true,
            .guidance_valid = true,
            .line = IR_LINE_VISIBLE};
}
static MissionOutputs tick(Mission *m, MissionInputs *in)
{
    MissionOutputs out;
    in->now_ms += 20;
    in->line_sampled_ms = in->now_ms;
    mission_step(m, in, &out);
    return out;
}
static void follow(Mission *m, MissionInputs *in)
{
    mission_init(m, in->now_ms);
    in->start_pressed = true;
    tick(m, in);
    in->start_pressed = false;
    assert(m->state == MISSION_FOLLOW_LINE);
}
static void move(MissionInputs *in, float left, float right)
{
    in->wheel_count[0] =
        enc_advance(in->wheel_count[0], (int)lroundf(left * ENC_COUNTS_PER_MM));
    in->wheel_count[1] =
        enc_advance(in->wheel_count[1], (int)lroundf(right * ENC_COUNTS_PER_MM));
}
static MissionOutputs junction(Mission *m, MissionInputs *in, int action)
{
    assert(mission_queue_direction(m, action));
    in->line = IR_LINE_BROAD_BLACK;
    in->left_black = in->right_black = true;
    tick(m, in);
    assert(m->state == MISSION_FOLLOW_LINE &&
        m->held == (barcode_action_t)action);
    move(in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
    return tick(m, in);
}
/* A straight crossing rolls its distance over the bar, then tracks. */
static MissionOutputs finish_straight(Mission *m, MissionInputs *in)
{
    assert(m->state == MISSION_NAVIGATE && m->phase == JUNCTION_PHASE_ADVANCE);
    move(in, NAV_STRAIGHT_MM + 1, NAV_STRAIGHT_MM + 1);
    return tick(m, in);
}

static void tracking(void)
{
    Mission m;
    MissionInputs in = input();
    follow(&m, &in);
    in.line_error = -127;
    MissionOutputs out = tick(&m, &in);
    assert(motion_cmd_steering(&out.command) == -LINE_KP_Q);
    move(&in, 5, 5);
    mission_step(&m, &in, &out); /* a repeated sample changes nothing */
    assert(m.line_steer_q == -LINE_KP_Q);
    /* Both black requests no steering: a crossbar must not keep a turn going. */
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    out = tick(&m, &in);
    assert(motion_cmd_steering(&out.command) == 0);
    assert(m.line_i_q == (int)lroundf(5.0f * LINE_KI_QPMM)); /* I froze at 5 mm */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    /* White/white drifts gently toward the side last touched. */
    in.line_error = 0;
    tick(&m, &in);
    assert(m.line_steer_q == -LINE_DRIFT_Q);
    /* A long touch grows I by distance, up to its limit. */
    in.line_error = -127;
    tick(&m, &in);
    move(&in, 20, 20);
    tick(&m, &in);
    assert(m.line_steer_q == -(int)lroundf(LINE_KP_Q + m.line_i_q + 20.0f * LINE_KI_QPMM));
    move(&in, 2.0f * LINE_I_MAX_Q, 2.0f * LINE_I_MAX_Q);
    tick(&m, &in);
    assert(m.line_steer_q == -(LINE_KP_Q + LINE_I_MAX_Q));
    /* White/white fades I by distance, down to the drift. */
    in.line_error = 0;
    tick(&m, &in);
    assert(m.line_steer_q == -LINE_I_MAX_Q && m.line_i_q == LINE_I_MAX_Q);
    move(&in, 20, 20);
    tick(&m, &in);
    assert(m.line_steer_q == -(int)lroundf(LINE_I_MAX_Q - 20.0f * LINE_I_FADE_QPMM));
    /* The same side resumes from the faded I, and does not jump to the limit. */
    in.line_error = -127;
    tick(&m, &in);
    assert(m.line_steer_q == -(int)lroundf(LINE_KP_Q + LINE_I_MAX_Q - 20.0f * LINE_I_FADE_QPMM));
    /* The other side touching clears I at once (S-bend). */
    in.line_error = 127;
    tick(&m, &in);
    assert(m.line_i_q == 0 && m.line_steer_q == LINE_KP_Q);
    for (int i = 0; i < 100; i++)
        tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE);
    in.now_ms += IR_MAX_GAP_MS + 1;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_STOPPED);
}
static void navigation(void)
{
    for (int action = BARCODE_LEFT; action <= BARCODE_U_TURN; action++)
    {
        Mission m;
        MissionInputs in = input();
        follow(&m, &in);
        MissionOutputs out = junction(&m, &in, action);
#if NAV_GEOMETRY_RELEASED
        assert(m.state == MISSION_NAVIGATE && m.held == BARCODE_UNMAPPED);
        if (action != BARCODE_STRAIGHT)
        {
            /* Every turn first advances the axle over the crossing. */
            assert(m.phase == JUNCTION_PHASE_ADVANCE);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
            assert(motion_cmd_steering(&out.command) == MOTION_TURN_RAMPED);
            assert(motion_cmd_target(&out.command) > 0);
            assert(motion_cmd_speed(&out.command) == COURSE_SPEED_MMPS);
        }
        in.stop_down = true;
        out = tick(&m, &in);
        assert(m.state == MISSION_STOPPED && out.stop_now);
#else
        if (action == BARCODE_STRAIGHT)
        {
            assert(m.state == MISSION_NAVIGATE && !out.stop_now);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
            continue;
        }
        assert(m.state == MISSION_STOPPED && out.stop_now);
        assert(out.nav_rejected == 1u);
#endif
    }
    /* A short black patch never consumes the command. */
    Mission m;
    MissionInputs in = input();
    follow(&m, &in);
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    in.line = IR_LINE_BROAD_BLACK;
    in.left_black = in.right_black = true;
    tick(&m, &in);
    in.line = IR_LINE_VISIBLE;
    in.left_black = in.right_black = false;
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.held == BARCODE_LEFT);
    /* A centred straight crossing needs no turn geometry or edge excursion. */
    for (int action = BARCODE_UNMAPPED; action <= BARCODE_STRAIGHT;
        action += BARCODE_STRAIGHT)
    {
        in = input();
        follow(&m, &in);
        if (action != BARCODE_UNMAPPED)
        {
            assert(mission_queue_direction(&m, action));
        }
        in.line = IR_LINE_BROAD_BLACK;
        in.left_black = in.right_black = true;
        tick(&m, &in);
        move(&in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
        MissionOutputs out = tick(&m, &in);
        assert(m.state == MISSION_NAVIGATE && !out.stop_now);
        assert(m.held == BARCODE_UNMAPPED && out.nav_rejected == 0u);
        /* Rolls straight over the bar at course speed, never stopping. */
        assert(m.phase == JUNCTION_PHASE_ADVANCE && !out.stop_now);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_steering(&out.command) == 0);
        assert(motion_cmd_speed(&out.command) == COURSE_SPEED_MMPS);
        in.line = IR_LINE_VISIBLE;
        in.left_black = in.right_black = false;
        out = finish_straight(&m, &in);
        assert(m.state == MISSION_FOLLOW_LINE && m.junction_locked);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) > 0);
    }
    /* A one-black exit still gets normal tracking guidance. */
    in = input();
    follow(&m, &in);
    junction(&m, &in, BARCODE_STRAIGHT);
    in.line = IR_LINE_VISIBLE;
    in.left_black = true;
    in.right_black = false;
    in.line_error = -127;
    tick(&m, &in);
    assert(m.state == MISSION_NAVIGATE); /* the trailing sensor never steers */
    in.left_black = false;
    in.line_error = 0;
    finish_straight(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.junction_locked);
    /* Both black on a curve (inner sensor black long before the outer) keeps
       the turn and never becomes an unqueued crossing. */
    in = input();
    follow(&m, &in);
    in.line_error = 127;
    in.right_black = true;
    for (int i = 0; i < 20; i++)
    {
        move(&in, 1, 1);
        tick(&m, &in);
    }
    int curve_q = m.line_steer_q;
    in.line = IR_LINE_BROAD_BLACK;
    in.left_black = true;
    in.guidance_valid = false;
    MissionOutputs out = tick(&m, &in);
    assert(m.entry_curved);
    move(&in, NAV_CONFIRM_MM + 5, NAV_CONFIRM_MM + 5);
    out = tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && motion_cmd_steering(&out.command) == curve_q);
    in.line = IR_LINE_VISIBLE;
    in.left_black = in.right_black = false;
    in.guidance_valid = true;
    in.line_error = 0;
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.line_steer_q > 0);
#if NAV_GEOMETRY_RELEASED
    /* A U-turn pause cannot run while wheels are moving. */
    in = input();
    follow(&m, &in);
    junction(&m, &in, BARCODE_U_TURN);
    in.motion_action = m.current_action;
    in.motion_status = RD_MOTION_DONE;
    tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_PAUSE_WAIT);
    in.motion_status = RD_MOTION_IDLE;
    for (int i = 0; i < 10; i++)
        tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_PAUSE_WAIT);
    in.stationary = true;
    tick(&m, &in);
    for (int i = 0; i < (int)(NAV_PAUSE_MS / 20u) - 1; i++)
        tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_PAUSE_WAIT);
    tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_TURN);
#endif
}
#if NAV_GEOMETRY_RELEASED
/* Rotate the robot in place by degrees, + left, as encoder counts. */
static void pivot(MissionInputs *in, float degrees)
{
    float arc = degrees / 57.2957795f * TRACK_WIDTH_MM * 0.5f;
    move(in, -arc, arc);
}
/* Commit, finish the advance, pause stationary: the pivot has started. */
static void to_pivot(Mission *m, MissionInputs *in, int action)
{
    junction(m, in, action);
    in->line = IR_LINE_VISIBLE;
    in->left_black = in->right_black = false;
    in->motion_action = m->current_action;
    in->motion_status = RD_MOTION_DONE;
    tick(m, in);
    assert(m->phase == JUNCTION_PHASE_PAUSE_WAIT);
    in->stationary = true;
    MissionOutputs out = {0};
    for (int i = 0; i < 20 && m->phase != JUNCTION_PHASE_TURN; i++)
        out = tick(m, in);
    assert(m->phase == JUNCTION_PHASE_TURN && motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    float bound = action == BARCODE_U_TURN ? NAV_UTURN_MAX_DEG : NAV_TURN_MAX_DEG;
    assert(motion_cmd_target(&out.command) ==
        (int)(action == BARCODE_LEFT ? bound : -bound));
    assert(motion_cmd_steering(&out.command) == MOTION_TURN_RAMPED);
    assert(motion_cmd_speed(&out.command) == COURSE_SPEED_MMPS);
    in->stationary = false;
    in->motion_action = m->current_action;
    in->motion_status = RD_MOTION_RUNNING;
}
/* The pivot ends on the new branch: leading sensor black, then not black,
   only past the sense angle; then it brakes, pauses and follows. */
static void pivot_turns(void)
{
    for (int action = BARCODE_LEFT; action <= BARCODE_U_TURN; action++)
    {
        if (action == BARCODE_STRAIGHT)
            continue;
        Mission m;
        MissionInputs in = input();
        follow(&m, &in);
        to_pivot(&m, &in, action);
        float sign = action == BARCODE_LEFT ? 1.0f : -1.0f;
        float sense = action == BARCODE_U_TURN ? NAV_UTURN_SENSE_DEG : NAV_TURN_SENSE_DEG;
        bool *leading = action == BARCODE_LEFT ? &in.left_black : &in.right_black;
        /* The branch being left crosses the sensor early: it never counts. */
        pivot(&in, sign * 20.0f);
        *leading = true;
        tick(&m, &in);
        *leading = false;
        MissionOutputs out = tick(&m, &in);
        assert(m.phase == JUNCTION_PHASE_TURN && motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        pivot(&in, sign * (sense - 10.0f));
        *leading = true;
        out = tick(&m, &in);
        assert(m.phase == JUNCTION_PHASE_TURN && motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        *leading = false;
        out = tick(&m, &in);
        assert(m.phase == JUNCTION_PHASE_BRAKE && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        assert(m.state == MISSION_NAVIGATE && !out.stop_now);
        in.stationary = true;
        for (int i = 0; i < 20 && m.state == MISSION_NAVIGATE; i++)
            out = tick(&m, &in);
        assert(m.state == MISSION_FOLLOW_LINE && m.junction_locked);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        /* INT-136: both sensors white after the pivot: lean back toward the line. */
        assert(motion_cmd_steering(&out.command) ==
            (action == BARCODE_LEFT ? LINE_TURN_LEAN_Q : -LINE_TURN_LEAN_Q));
    }
}
#endif
/* INT-139: a straight crossing that exits white/white keeps the last side, so
   the robot drifts back toward the line with line loss armed. */
static void crossing_keeps_last_side(void)
{
    Mission m;
    MissionInputs in = input();
    follow(&m, &in);
    in.line = IR_LINE_VISIBLE;
    in.left_black = true;
    in.line_error = -IR_SCALE_ONE;
    tick(&m, &in);
    assert(m.line_side == -1);
    move(&in, 5.0f, 5.0f); /* within the onset skew, so this is a crossing */
    in.line = IR_LINE_BROAD_BLACK;
    in.right_black = true;
    in.line_error = 0;
    tick(&m, &in);
    move(&in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
    tick(&m, &in);
    assert(m.state == MISSION_NAVIGATE);
    in.line = IR_LINE_VISIBLE;
    in.left_black = in.right_black = false;
    MissionOutputs out = finish_straight(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.line_side == -1);
    assert(motion_cmd_steering(&out.command) == -LINE_DRIFT_Q);
    move(&in, LINE_LOST_MM + 1.0f, LINE_LOST_MM + 1.0f);
    tick(&m, &in);
    assert(m.state == MISSION_REACQUIRE);
}
static void navigation_failures(void)
{
    for (unsigned phase = 0; phase <= JUNCTION_PHASE_ADVANCE; ++phase)
    {
        for (unsigned cause = 0; cause < 5; ++cause)
        {
            Mission m;
            MissionInputs in = input();
            follow(&m, &in);
            m.state = MISSION_NAVIGATE;
            m.phase = phase;
            if (cause == 0) in.stop_down = true;
            if (cause == 1) in.safety_latched = true;
            if (cause == 2) in.usable_line = false;
            if (cause == 3) { in.front_valid = true; in.front_mm = 100; }
            if (cause == 4) in.now_ms += MOTION_ACTION_TIMEOUT_MS;
            MissionOutputs out = tick(&m, &in);
            assert(m.state == MISSION_STOPPED);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        }
    }
#if NAV_GEOMETRY_RELEASED
    Mission m;
    MissionInputs in = input();
    follow(&m, &in);
    in.line_error = -127;
    in.left_black = true;
    for (unsigned i = 0; i < 12; ++i) /* inner sensor black well before the outer */
    {
        move(&in, 2, 2);
        tick(&m, &in);
    }
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    in.right_black = true;
    in.line = IR_LINE_BROAD_BLACK;
    tick(&m, &in);
    assert(m.entry_curved);
    move(&in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
    MissionOutputs out = tick(&m, &in);
    /* INT-153: a queued turn commits on a skewed but qualified entry. */
    assert(m.state == MISSION_NAVIGATE && m.phase == JUNCTION_PHASE_ADVANCE && out.nav_rejected == 0u);
    /* Pivoting to the bound without finding the branch stops. */
    in = input(); follow(&m, &in);
    to_pivot(&m, &in, BARCODE_LEFT);
    in.motion_status = RD_MOTION_DONE;
    tick(&m, &in);
    assert(m.state == MISSION_STOPPED);
#endif
}
static void motion(void)
{
    /* Style 2 was the removed smooth path; it is no longer a valid turn. */
    MotionCommand cmd = motion_command_make(MOTION_MODE_TURN, 50, 90, 2, 2, 100);
    assert(motion_cmd_mode(&cmd) == MOTION_MODE_STOP);
    cmd = motion_command_make(MOTION_MODE_TURN, 50, -NAV_UTURN_MAX_DEG,
        MOTION_TURN_RAMPED, 2, 100);
    assert(motion_cmd_mode(&cmd) == MOTION_MODE_TURN &&
        motion_cmd_target(&cmd) == -(int)NAV_UTURN_MAX_DEG);
}
int main(void)
{
    tracking();
    navigation();
    navigation_failures();
    crossing_keeps_last_side();
#if NAV_GEOMETRY_RELEASED
    pivot_turns();
#endif
    motion();
    puts("smooth_navigation: PASS (host only)");
}
