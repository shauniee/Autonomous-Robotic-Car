#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/subsystem2/smooth_path.h"
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
static void geometry(void)
{
    for (int sign = -1; sign <= 1; sign += 2)
    {
        float d = 150, xc = 25, yc = 7, b = 130,
            L = smooth_path_length(d, yc, sign);
        assert(smooth_path_valid(L, b));
        float x = -d, y = 0, theta = 0, prev[2] = {0, 0};
        for (int i = 1; i <= 10000; i++)
        {
            float p[2], r[2];
            smooth_path_reference(L, b, sign, L * i / 10000.0f, p, r);
            float dl = p[0] - prev[0], dr = p[1] - prev[1], dt = (dr - dl) / b;
            x += (dl + dr) * .5f * cosf(theta + dt * .5f);
            y += (dl + dr) * .5f * sinf(theta + dt * .5f);
            theta += dt;
            assert(r[0] >= 0 && r[1] >= 0);
            prev[0] = p[0];
            prev[1] = p[1];
        }
        assert(fabsf(x + xc * cosf(theta) - yc * sinf(theta)) < .05f);
        assert(fabsf(y + xc * sinf(theta) + yc * cosf(theta) -
            sign * (SMOOTH_PATH_C * L + xc)) < .05f);
        assert(fabsf(theta - sign * SMOOTH_PATH_PI / 2) < .001f);
        float p[2], r[2];
        smooth_path_reference(L, b, sign, 0, p, r);
        assert(r[0] == 1 && r[1] == 1);
        smooth_path_reference(L, b, sign, L, p, r);
        assert(r[0] == 1 && r[1] == 1);
    }
    assert(!smooth_path_valid(10, 130));
    assert(!smooth_path_valid(NAN, 130));
}
static void tracking(void)
{
    Mission m;
    MissionInputs in = input();
    follow(&m, &in);
    in.line_error = -127;
    MissionOutputs out = tick(&m, &in);
    assert(motion_cmd_steering(&out.command) == -(LINE_INITIAL_Q + LINE_BUILD_Q));
    mission_step(&m, &in, &out); /* a repeated sample cannot grow I */
    assert(m.line_steer_q == -(LINE_INITIAL_Q + LINE_BUILD_Q));
    tick(&m, &in);
    assert(m.line_steer_q == -(LINE_INITIAL_Q + 2 * LINE_BUILD_Q));
    /* Both black requests no steering: a crossbar must not keep a turn going. */
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    out = tick(&m, &in);
    assert(motion_cmd_steering(&out.command) == 0);
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 127;
    tick(&m, &in);
    assert(m.line_steer_q == LINE_INITIAL_Q - 2 * LINE_BUILD_Q + LINE_UNWIND_Q); /* I carries over */
    in.line_error = 0;
    for (int i = 0; i < 20; i++)
        tick(&m, &in);
    assert(m.line_steer_q == -2 * LINE_BUILD_Q + LINE_UNWIND_Q); /* P drops, I holds */
    /* An opposing touch drains the held integral by LINE_UNWIND_Q per sample. */
    {
        in.line_error = -127;
        for (int i = 0; i < 10; i++)
            tick(&m, &in);
        int before = m.line_i_q;
        in.line_error = 127;
        tick(&m, &in);
        assert(m.line_i_q == before + LINE_UNWIND_Q);
        in.line_error = 0;
        tick(&m, &in);
    }
#if LINE_UNWIND_SAMPLES < 50
    /* A brief opposite touch drains slowly; once it outlasts LINE_UNWIND_SAMPLES
       (a curve ending) the integral drains at LINE_EXIT_Q. */
    {
        in = input();
        follow(&m, &in);
        in.line_error = 127;
        for (int i = 0; i < 40; i++)
            tick(&m, &in);
        in.line_error = -127;
        int before = m.line_i_q;
        for (unsigned i = 0; i < LINE_UNWIND_SAMPLES; i++)
            tick(&m, &in);
        assert(m.line_i_q == before - (int)LINE_UNWIND_SAMPLES * LINE_UNWIND_Q);
        before = m.line_i_q;
        tick(&m, &in);
        assert(m.line_i_q == before - LINE_EXIT_Q);
        /* The exit drain stops at zero and holds for LINE_HOLD_SAMPLES of the same
           spell; a spell still black after that (an S-bend) builds normally. */
        while (m.line_i_q != 0)
            tick(&m, &in);
        for (unsigned i = 0; i < LINE_HOLD_SAMPLES; i++)
            tick(&m, &in);
        assert(m.line_i_q == 0 && m.line_steer_q == -LINE_HOLD_KICK_Q);
        tick(&m, &in);
        assert(m.line_i_q == -LINE_BUILD_Q);
        in = input();
        follow(&m, &in);
        in.line_error = 127;
        for (int i = 0; i < 40; i++)
            tick(&m, &in);
        in.line_error = -127;
        while (m.line_i_q != 0)
            tick(&m, &in);
        in.line_error = 0;
        tick(&m, &in);
        in.line_error = -127;
        tick(&m, &in);
        assert(m.line_i_q == -LINE_BUILD_Q); /* white/white released the hold */
        in.line_error = 0;
        tick(&m, &in);
    }
#endif
    /* A long one-black spell saturates only at the limit, never beyond. */
    in.line_error = 127;
    for (int i = 0; i < 200; i++)
        tick(&m, &in);
    assert(m.line_steer_q == 127 && m.line_i_q == 127 - LINE_INITIAL_Q);
    in.line_error = 0;
    tick(&m, &in);
    assert(m.line_steer_q == 127 - LINE_INITIAL_Q);
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
        if (action == BARCODE_LEFT || action == BARCODE_RIGHT)
        {
            assert(motion_cmd_steering(&out.command) == MOTION_TURN_SMOOTH);
            assert((motion_cmd_target(&out.command) > 0) == (action == BARCODE_LEFT));
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
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) > 0);
        assert(motion_cmd_steering(&out.command) == 0);
        in.line = IR_LINE_VISIBLE;
        in.left_black = in.right_black = false;
        out = tick(&m, &in);
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
    in.left_black = false;
    in.line_error = 0;
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.junction_locked);
    /* Both black on a curve (large held integral) keeps the turn and never
       becomes an unqueued crossing. */
    in = input();
    follow(&m, &in);
    in.line_error = 127;
    for (int i = 0; i < 20; i++)
        tick(&m, &in);
    int curve_q = m.line_steer_q;
    in.line = IR_LINE_BROAD_BLACK;
    in.left_black = in.right_black = true;
    in.guidance_valid = false;
    MissionOutputs out = tick(&m, &in);
    move(&in, NAV_CURVE_CONFIRM_MM + 5, NAV_CURVE_CONFIRM_MM + 5);
    out = tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && motion_cmd_steering(&out.command) == curve_q);
    in.line = IR_LINE_VISIBLE;
    in.left_black = in.right_black = false;
    in.guidance_valid = true;
    in.line_error = 0;
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.line_steer_q == m.line_i_q && m.line_i_q > 0);
    /* A crossing that skewed the robot steers back to the pre-crossing heading
       before white/white hands over to tracking. */
    in = input();
    follow(&m, &in);
    move(&in, 36, 0); /* about 17 degrees right while approaching */
    in.line = IR_LINE_BROAD_BLACK;
    in.left_black = in.right_black = true;
    tick(&m, &in);
    move(&in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
    out = tick(&m, &in);
    assert(m.state == MISSION_NAVIGATE && motion_cmd_steering(&out.command) < 0);
    in.line = IR_LINE_VISIBLE;
    in.left_black = in.right_black = false;
    out = tick(&m, &in);
    assert(m.state == MISSION_NAVIGATE && motion_cmd_steering(&out.command) < 0);
    move(&in, 0, 36);
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE && m.junction_locked);
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
    for (int i = 0; i < 4; i++)
        tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_PAUSE_WAIT);
    tick(&m, &in);
    assert(m.phase == JUNCTION_PHASE_TURN);
#endif
}
static void navigation_failures(void)
{
    for (unsigned phase = 0; phase <= JUNCTION_PHASE_EXIT; ++phase)
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
    for (unsigned i = 0; i < 12; ++i) tick(&m, &in); /* integral above the curve bound */
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    in.left_black = in.right_black = true;
    in.line = IR_LINE_BROAD_BLACK;
    tick(&m, &in);
    move(&in, NAV_CONFIRM_MM + 1, NAV_CONFIRM_MM + 1);
    tick(&m, &in);
    assert(m.state == MISSION_FOLLOW_LINE);
    move(&in, NAV_CURVE_CONFIRM_MM, NAV_CURVE_CONFIRM_MM);
    tick(&m, &in);
    assert(m.state == MISSION_STOPPED);
    /* Completion without return-line evidence cannot resume tracking. */
    in = input(); follow(&m, &in);
    junction(&m, &in, BARCODE_LEFT);
    in.motion_action = m.current_action;
    in.motion_status = RD_MOTION_DONE;
    tick(&m, &in);
    assert(m.state == MISSION_STOPPED);
#endif
}
static void motion(void)
{
    MotionCommand cmd = motion_command_make(MOTION_MODE_TURN, 50, 250,
        MOTION_TURN_SMOOTH, 2, 100);
#if NAV_GEOMETRY_RELEASED
    assert(motion_cmd_mode(&cmd) == MOTION_MODE_TURN);
    MotionController c;
    motion_control_reset(&c);
    MotionInput in = {.now_ms = 100, .count = {65530, 65530}};
    MotionOutput out;
    /* Simulated exact wheel-speed following verifies reference coordination. */
    for (unsigned i = 0; i < 350; i++)
    {
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&c, &cmd, &in, &out);
        assert(out.status != RD_MOTION_FAILED);
        uint32_t held_reference = c.path_q8;
        motion_control_step(&c, &cmd, &in, &out);
        assert(c.path_q8 == held_reference);
        if (out.status == RD_MOTION_DONE)
        {
            break;
        }
        in.count[0] =
            enc_advance(in.count[0], (int)lroundf(out.target_cps[0] * .020f));
        in.count[1] =
            enc_advance(in.count[1], (int)lroundf(out.target_cps[1] * .020f));
        in.now_ms += 20;
    }
    assert(out.status == RD_MOTION_DONE && out.target_cps[0] > 0 &&
        out.target_cps[1] > 0);
    in.now_ms += MOTION_EXPIRY_MS + 1;
    motion_control_step(&c, &cmd, &in, &out);
    assert(out.duty[0] == 0 && out.duty[1] == 0);
    motion_control_reset(&c);
    in.now_ms = 100;
    for (unsigned i = 0; i < 700; i++)
    {
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&c, &cmd, &in, &out);
        in.now_ms += 20;
        if (out.status == RD_MOTION_FAILED)
        {
            break;
        }
    }
    /* No encoder progress cannot drag the reference beyond its lead bound. */
    assert(c.path_q8 / 256.0f <= PATH_LEAD_MM + .01f);
    assert(out.status == RD_MOTION_FAILED);
    motion_control_reset(&c);
    in.now_ms = 100; cmd.sampled_ms = 100;
    motion_control_step(&c, &cmd, &in, &out);
    cmd.target = 300;
    in.now_ms += 20; cmd.sampled_ms = in.now_ms;
    motion_control_step(&c, &cmd, &in, &out);
    assert(out.status == RD_MOTION_FAILED);
    assert(out.duty[0] == 0 && out.duty[1] == 0);
#else
    assert(motion_cmd_mode(&cmd) == MOTION_MODE_STOP);
#endif
}
int main(void)
{
    geometry();
    tracking();
    navigation();
    navigation_failures();
    motion();
    puts("smooth_navigation: PASS (host only)");
}
