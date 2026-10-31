/* Host test for app_program/mission/mission_logic.[ch] (issue #36). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/common/robot_time_math.h"

static MissionInputs base(uint32_t now)
{
    return (MissionInputs){.now_ms = now, .inputs_ok = true, .guidance_valid = true,
                           .usable_line = true, .line = IR_LINE_VISIBLE, .line_sampled_ms = now,
                           .motion_stopped = true, .motion_status = RD_MOTION_IDLE, .mapped = true};
}

static MissionOutputs step(Mission *m, MissionInputs *in, uint32_t now)
{
    MissionOutputs out;
    in->now_ms = now;
    in->line_sampled_ms = now;
    mission_step(m, in, &out);
    return out;
}

static int ref_initial_steer(int error_q)
{
    int correction = IR_SCALE_ONE;
    return error_q < 0 ? -correction : (error_q > 0 ? correction : 0);
}

static unsigned ref_speed(int steer_q)
{
    return (unsigned)floorf(MISSION_TRACK_SPEED_MMPS *
        1.0f / (1.0f + 0.60f * abs(steer_q) / 127.0f));
}

/* Drive a Mission to FOLLOW through the deliberate start press. */
static uint32_t to_follow(Mission *m, MissionInputs *in)
{
    uint32_t t = 100u;
    mission_init(m, t);
    MissionOutputs out = step(m, in, t);			/* calibrated */
    assert(m->state == MISSION_READY && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in->motion_stopped = false; /* Boot reset already serviced. */
    t += 10u; in->start_pressed = true;
    out = step(m, in, t);					/* start press */
    assert(m->state == MISSION_FOLLOW_LINE && out.start_scanner);
    in->start_pressed = false;
    return t;
}

static void test_start_and_reset_never_move(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* A press while stopped only releases the latch; no motion command results. */
    in.stop_down = true; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_STOPPED && out.stop_now && out.stop_scanner && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.stop_down = false; in.motion_stopped = true; in.start_pressed = true; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_STOPPED && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.start_pressed = false; in.motion_stopped = false; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);	/* still waiting for a start press */

    /* A start press that Motion never honours times out instead of following. */
    in.motion_stopped = true; in.start_pressed = true; t += 10u;
    step(&m, &in, t);
    in.start_pressed = false; t += MISSION_START_WAIT_MS;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
}

static void test_start_gate(void)
{
    /* Each prerequisite can keep READY even after GP20 is accepted. */
    for (unsigned blocker = 0u; blocker < 4u; ++blocker)
    {
        Mission m;
        MissionInputs in = base(100u);
        mission_init(&m, 100u);
        in.motion_stopped = false;
        in.start_pressed = true;
        if (blocker == 0u)
        {
            in.mapped = false;
        }
        else if (blocker == 1u)
        {
            in.start_blocked = true;
        }
        else if (blocker == 2u)
        {
            in.motion_stopped = true;
        }
        else
        {
            in.guidance_valid = false;
        }
        MissionOutputs out = step(&m, &in, 110u);
        assert(m.state == MISSION_READY && out.start_refused);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        in.mapped = true;
        in.start_blocked = false;
        in.motion_stopped = false;
        in.guidance_valid = true;
        in.start_pressed = false;
        out = step(&m, &in, 120u);
        assert(m.state == MISSION_READY && !out.start_scanner);
        in.start_pressed = true;
        out = step(&m, &in, 130u);
        assert(m.state == MISSION_FOLLOW_LINE && out.start_scanner);
    }
}

static void test_follow_and_stops(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line_error = IR_SCALE_ONE; t += 10u;			/* right black: steer left */
    MissionOutputs out = step(&m, &in, t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW && motion_cmd_steering(&out.command) == ref_initial_steer(IR_SCALE_ONE));
    assert(out.command.sampled_ms == t && motion_cmd_speed(&out.command) == ref_speed(ref_initial_steer(IR_SCALE_ONE)));
    assert(ref_speed(0) == (unsigned)MISSION_TRACK_SPEED_MMPS &&
        ref_speed(ref_initial_steer(IR_SCALE_ONE)) < ref_speed(0));

    /* Unusable inputs or a Safety latch stop; Safety's own stop is not repeated. */
    in.inputs_ok = false; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    Mission m2; MissionInputs in2 = base(100u);
    t = to_follow(&m2, &in2);
    in2.safety_latched = true; t += 10u;
    out = step(&m2, &in2, t);
    assert(m2.state == MISSION_STOPPED && !out.stop_now && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
}

static void advance_to_turn(Mission *m, MissionInputs *in, uint32_t *t)
{
    if (m->phase == JUNCTION_PHASE_ADVANCE)
    {
        int counts = (int)ceilf(MISSION_TURN_ADVANCE_MM * ENC_COUNTS_PER_MM);
        in->wheel_count[0] = enc_advance(in->wheel_count[0], counts);
        in->wheel_count[1] = enc_advance(in->wheel_count[1], counts);
        MissionOutputs out = step(m, in, *t += IR_SAMPLE_MS);
        assert(m->phase == JUNCTION_PHASE_TURN);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    }
}

static void cross_junction(Mission *m, MissionInputs *in, uint32_t *t, uint32_t length)
{
    in->line = IR_LINE_BROAD_BLACK;
    in->guidance_valid = false;
    in->wheel_count[0] = in->wheel_count[1] = 1000u;
    *t += 10u; step(m, in, *t);
    if (m->state == MISSION_NAVIGATE)
    {
        advance_to_turn(m, in, t);
        in->line = IR_LINE_VISIBLE;
        in->guidance_valid = true;
        return;
    }
    in->motion_action = 1u;
    in->motion_status = RD_MOTION_RUNNING;
    in->wheel_count[0] += length;
    in->wheel_count[1] += length;
    *t += 10u; step(m, in, *t);
    in->line = IR_LINE_VISIBLE;
    in->guidance_valid = true;
}

static void test_both_black_forward(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in) + 10u;
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_now);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    assert(motion_cmd_target(&out.command) == 0);
    assert(motion_cmd_steering(&out.command) == 0);
    assert(motion_cmd_speed(&out.command) == (unsigned)MISSION_BROAD_SPEED_MMPS);
    uint8_t id = motion_cmd_action(&out.command);
    in.now_ms = t + 10u;
    mission_step(&m, &in, &out);
    assert(out.command.sampled_ms == t && motion_cmd_action(&out.command) == id);
    /* A thin patch ends immediately; there is no distance action to finish. */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.motion_action = id;
    in.motion_status = RD_MOTION_RUNNING;
    out = step(&m, &in, t + 20u);
    assert(m.state == MISSION_FOLLOW_LINE && !m.in_broad);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    out = step(&m, &in, t + 30u);
    assert(m.state == MISSION_FOLLOW_LINE && m.in_broad);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);

    /* Stop, unusable sample, obstacle and deadline still stop an encounter. */
    for (unsigned scenario = 0u; scenario < 4u; ++scenario)
    {
        in = base(100u);
        to_follow(&m, &in);
        t = UINT32_MAX - 100u;
        in.line = IR_LINE_BROAD_BLACK;
        in.guidance_valid = false;
        step(&m, &in, t);
        if (scenario == 0u)
        {
            in.stop_down = true;
        }
        else if (scenario == 1u)
        {
            in.usable_line = false;
        }
        else if (scenario == 2u)
        {
            in.front_valid = true;
            in.front_mm = MISSION_OBSTACLE_STOP_MM;
        }
        else
        {
            out = step(&m, &in, t + MOTION_ACTION_TIMEOUT_MS - 1u);
            assert(m.state == MISSION_FOLLOW_LINE);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
            t += MOTION_ACTION_TIMEOUT_MS - 10u;
        }
        out = step(&m, &in, t + 10u);
        assert(m.state == MISSION_STOPPED && out.stop_now);
    }
    puts("INT-39: no fixed junction advance; encounter guards PASS");
}

static void test_barcode_hold_and_junction(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* A barcode is held, not executed, until the next cross junction. */
    in.have_barcode = true; in.barcode_action = BARCODE_LEFT; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_FOLLOW_LINE && motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW && m.held == BARCODE_LEFT);
    in.have_barcode = false;

    cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_NAVIGATE && motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == 360.0f);
    uint8_t turn_id = motion_cmd_action(&out.command);
    assert(turn_id >= 2u && m.held == BARCODE_UNMAPPED);

    /* IR completion must release NAVIGATE without turn-DONE or clear-DONE. */
    in.motion_status = RD_MOTION_RUNNING;
    in.motion_action = turn_id;
    in.left_black = false;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.turn_line_phase == 1u);
    in.left_black = true;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.turn_line_phase == 2u);
    in.left_black = false;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.phase == JUNCTION_PHASE_BRAKE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = step(&m, &in, t += FOLLOW_PERIOD_MS);
    assert(m.state == MISSION_NAVIGATE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.motion_status = RD_MOTION_IDLE;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.state == MISSION_FOLLOW_LINE);
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);

    /* Right and U-turn targets; an unused junction with nothing held carries straight on. */
    struct { barcode_action_t a; float deg; } turns[] = {{BARCODE_RIGHT, -360.0f}, {BARCODE_U_TURN, -360.0f}};
    for (unsigned i = 0; i < 2u; ++i) {
        Mission mm; MissionInputs ii = base(100u);
        t = to_follow(&mm, &ii);
        ii.have_barcode = true; ii.barcode_action = turns[i].a; t += 10u; step(&mm, &ii, t);
        ii.have_barcode = false;
        cross_junction(&mm, &ii, &t, MISSION_JUNCTION_MIN_COUNTS + 3u);
        t += 10u;
        out = step(&mm, &ii, t);
        assert(mm.state == MISSION_NAVIGATE);
        if (turns[i].a == BARCODE_U_TURN)
        {
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
            ii.stationary = true;
            out = step(&mm, &ii, t += 10u);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
            out = step(&mm, &ii, t += MISSION_UTURN_PAUSE_MS - 1u);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
            out = step(&mm, &ii, ++t);
        }
        assert(motion_cmd_target(&out.command) == turns[i].deg);
    }
    Mission ms; MissionInputs is = base(100u);
    t = to_follow(&ms, &is);
    cross_junction(&ms, &is, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u; step(&ms, &is, t);
    assert(ms.state == MISSION_NAVIGATE && ms.phase == JUNCTION_PHASE_STRAIGHT);
    unsigned straight_counts = (unsigned)ceilf(MISSION_STRAIGHT_MM * ENC_COUNTS_PER_MM);
    is.wheel_count[0] += straight_counts;
    is.wheel_count[1] += straight_counts;
    step(&ms, &is, t += 20u);
    assert(ms.state == MISSION_FOLLOW_LINE);
    /* A failed turn stops. */
    Mission mf; MissionInputs iff = base(100u);
    t = to_follow(&mf, &iff);
    iff.have_barcode = true; iff.barcode_action = BARCODE_LEFT; t += 10u; step(&mf, &iff, t); iff.have_barcode = false;
    cross_junction(&mf, &iff, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u; out = step(&mf, &iff, t);
    iff.motion_action = motion_cmd_action(&out.command); iff.motion_status = RD_MOTION_FAILED; t += 10u;
    out = step(&mf, &iff, t);
    assert(mf.state == MISSION_STOPPED && out.stop_now);
}

static void test_crossing_never_completes_mission(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    in.wheel_count[0] = in.wheel_count[1] = 0xFFF0u;
    step(&m, &in, t += 10u);
    in.wheel_count[0] = in.wheel_count[1] = (0xFFF0u + 120u) & ENC_COUNT_MASK;
    MissionOutputs out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_scanner);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    in.motion_action = m.current_action;
    in.motion_status = RD_MOTION_DONE;
    out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_FOLLOW_LINE && m.in_broad);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    assert(motion_cmd_steering(&out.command) == 0 && !out.stop_scanner);
    out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_FOLLOW_LINE); /* one action per black encounter */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    step(&m, &in, t += 10u);
    assert(m.state == MISSION_NAVIGATE && !m.in_broad);
    unsigned straight_counts = (unsigned)ceilf(MISSION_STRAIGHT_MM * ENC_COUNTS_PER_MM);
    in.wheel_count[0] = enc_advance(in.wheel_count[0], straight_counts);
    in.wheel_count[1] = enc_advance(in.wheel_count[1], straight_counts);
    step(&m, &in, t += 20u);
    assert(m.state == MISSION_FOLLOW_LINE);
}

static void test_straight_junction_segment(void)
{
    for (unsigned scenario = 0u; scenario < 10u; ++scenario)
    {
        Mission m;
        MissionInputs in = base(100u);
        uint32_t t = to_follow(&m, &in);
        if (scenario == 1u)
        {
            assert(mission_queue_direction(&m, BARCODE_STRAIGHT));
        }
        cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
        in.wheel_count[0] = in.wheel_count[1] = ENC_COUNT_MASK - 8u;
        if (scenario == 1u)
        {
            m.broad_origin[0] = m.broad_origin[1] = in.wheel_count[0];
        }
        in.line = IR_LINE_LOST;
        in.guidance_valid = false;
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_NAVIGATE && m.phase == JUNCTION_PHASE_STRAIGHT);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) == (unsigned)MISSION_TRACK_SPEED_MMPS);
        assert(motion_cmd_steering(&out.command) == 0);
        assert(m.held == BARCODE_UNMAPPED && !m.manual_held);
        uint32_t origin = in.wheel_count[0];
        unsigned counts = (unsigned)ceilf(MISSION_STRAIGHT_MM * ENC_COUNTS_PER_MM);
        if (scenario < 2u)
        {
            /* Both angled exits steer before the measured segment finishes. */
            in.line = IR_LINE_VISIBLE;
            in.guidance_valid = true;
            in.line_error = scenario == 0u ? -127 : 127;
            const int expected[] = {127, 127, 127, 127};
            for (unsigned i = 0u; i < 4u; ++i)
            {
                out = step(&m, &in, t += IR_SAMPLE_MS);
                assert(m.state == MISSION_NAVIGATE);
                int steering = scenario == 0u ? -expected[i] : expected[i];
                assert(motion_cmd_steering(&out.command) == steering);
                assert(motion_cmd_speed(&out.command) == ref_speed(steering));
                in.now_ms = t + FOLLOW_PERIOD_MS;
                mission_step(&m, &in, &out);
                assert(motion_cmd_steering(&out.command) == steering);
            }
            in.wheel_count[0] = enc_advance(origin, counts - 1u);
            in.wheel_count[1] = enc_advance(origin, counts - 1u);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.state == MISSION_NAVIGATE);
            in.wheel_count[0] = in.wheel_count[1] = enc_advance(origin, counts);
            if (scenario == 0u)
            {
                in.line = IR_LINE_LOST;
                in.guidance_valid = false;
            }
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(!out.stop_now && !out.line_lost);
            if (scenario == 1u)
            {
                assert(m.state == MISSION_FOLLOW_LINE);
                assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
                assert(motion_cmd_steering(&out.command) == 127);
            }
            else
            {
                assert(m.state == MISSION_FOLLOW_LINE);
                assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
                assert(m.line_loss_pending && !out.stop_scanner);
            }
            continue;
        }
        if (scenario == 2u)
        {
            in.hump_active = true;
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(motion_cmd_speed(&out.command) == (unsigned)floorf(MISSION_TRACK_SPEED_MMPS * 0.5f));
            continue;
        }
        if (scenario == 3u)
        {
            in.stop_down = true;
        }
        else if (scenario == 4u)
        {
            in.front_valid = true;
            in.front_mm = MISSION_OBSTACLE_STOP_MM;
        }
        else if (scenario == 5u)
        {
            in.usable_line = false;
        }
        else if (scenario == 6u)
        {
            in.now_ms += IR_MAX_GAP_MS + 1u;
            mission_step(&m, &in, &out);
            assert(m.state == MISSION_STOPPED && out.stop_now);
            continue;
        }
        else if (scenario == 7u)
        {
            t = m.state_since_ms + MOTION_ACTION_TIMEOUT_MS;
        }
        else if (scenario == 8u)
        {
            /* One wheel running ahead cannot finish before mean travel. */
            in.wheel_count[0] = enc_advance(origin, counts);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.state == MISSION_NAVIGATE);
            in.wheel_count[0] = in.wheel_count[1] = enc_advance(origin, -(int)counts);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.state == MISSION_NAVIGATE);
            continue;
        }
        else
        {
            in.motion_stopped = true;
        }
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_STOPPED);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    }
    puts("INT-61: 25 mm ramped straight exit, wrap, no visible gate and safety guards PASS");
}

static MissionOutputs enter_line_recovery(Mission *m, MissionInputs *in,
    uint32_t *t)
{
    in->line = IR_LINE_LOST;
    in->guidance_valid = false;
    MissionOutputs out = step(m, in, *t += IR_SAMPLE_MS);
    assert(m->state == MISSION_FOLLOW_LINE);
    for (unsigned elapsed = IR_SAMPLE_MS; elapsed <= MISSION_LINE_LOST_MS;
        elapsed += IR_SAMPLE_MS)
    {
        out = step(m, in, *t += IR_SAMPLE_MS);
    }
    assert(m->state == MISSION_REACQUIRE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    return out;
}

static void test_line_loss_qualification(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line_error = 80;
    MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
    int steering = motion_cmd_steering(&out.command);
    unsigned speed = motion_cmd_speed(&out.command);
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    uint32_t first = t;
    for (unsigned elapsed = 0u; elapsed < MISSION_LINE_LOST_MS;
        elapsed += IR_SAMPLE_MS)
    {
        out = step(&m, &in, t = first + elapsed);
        assert(m.state == MISSION_FOLLOW_LINE && m.line_loss_pending);
        assert(motion_cmd_steering(&out.command) == steering);
        assert(motion_cmd_speed(&out.command) == speed);
        assert(out.command.sampled_ms == in.line_sampled_ms);
        assert(!out.stop_scanner && !out.scan_cancel);
        assert(m.held == BARCODE_LEFT && m.manual_held);
    }
    out = step(&m, &in, t = first + MISSION_LINE_LOST_MS - 1u);
    assert(m.state == MISSION_FOLLOW_LINE);
    out = step(&m, &in, t = first + MISSION_LINE_LOST_MS);
    assert(m.state == MISSION_REACQUIRE && !m.line_loss_pending);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    assert(out.stop_scanner && out.scan_cancel && !out.side_watch_on);
    assert(m.held == BARCODE_UNMAPPED && !m.manual_held);

    /* Visibility/broad-black resets qualification, even near the deadline. */
    for (unsigned scenario = 0u; scenario < 2u; ++scenario)
    {
        in = base(100u);
        t = to_follow(&m, &in);
        in.line = IR_LINE_LOST;
        in.guidance_valid = false;
        for (unsigned i = 0u; i < 20u; ++i)
        {
            out = step(&m, &in, t += IR_SAMPLE_MS);
        }
        in.line = scenario == 0u ? IR_LINE_VISIBLE : IR_LINE_BROAD_BLACK;
        in.guidance_valid = scenario == 0u;
        step(&m, &in, t += IR_SAMPLE_MS);
        assert(!m.line_loss_pending);
        in.line = IR_LINE_LOST;
        in.guidance_valid = false;
        step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(m.line_lost_since_ms == t);
    }

    /* Both zero acquisition time and wrap through zero qualify correctly. */
    const uint32_t starts[] = {0u, UINT32_MAX - 200u};
    for (unsigned i = 0u; i < 2u; ++i)
    {
        in = base(100u);
        to_follow(&m, &in);
        t = starts[i] - IR_SAMPLE_MS;
        enter_line_recovery(&m, &in, &t);
        assert(m.state == MISSION_REACQUIRE);
    }

    /* A missing interval does not count; a cached sample becomes stale. */
    in = base(100u);
    t = to_follow(&m, &in);
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    step(&m, &in, t += IR_SAMPLE_MS);
    out = step(&m, &in, t += MISSION_LINE_LOST_MS);
    assert(m.state == MISSION_FOLLOW_LINE && m.line_lost_since_ms == t);
    in.now_ms += IR_MAX_GAP_MS;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_FOLLOW_LINE);
    ++in.now_ms;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(!m.line_loss_pending);

    for (unsigned scenario = 0u; scenario < 3u; ++scenario)
    {
        in = base(100u);
        t = to_follow(&m, &in);
        in.line = IR_LINE_LOST;
        in.guidance_valid = false;
        step(&m, &in, t += IR_SAMPLE_MS);
        if (scenario == 0u)
        {
            in.stop_down = true;
        }
        else if (scenario == 1u)
        {
            in.usable_line = false;
        }
        else
        {
            in.inputs_ok = false;
        }
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_STOPPED && out.stop_now);
    }
    puts("INT-63: 499/500 ms loss, reset, queue retention, freshness and wrap PASS");
}

static void test_line_loss_recovery_is_bounded(void)
{
    for (int error = -100; error <= 100; error += 200)
    {
        Mission m;
        MissionInputs in = base(100u);
        uint32_t t = to_follow(&m, &in);
        enter_line_recovery(&m, &in, &t);
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        in.stationary = true;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        assert(motion_cmd_target(&out.command) == 70);
        MotionController controller;
        motion_control_reset(&controller);
        MotionInput input = {.now_ms = t};
        MotionOutput motion;
        motion_control_step(&controller, &out.command, &input, &motion);
        assert(motion.status == RD_MOTION_RUNNING);
        /* Broad black is not usable guidance; either visible side is. */
        in.line = IR_LINE_BROAD_BLACK;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_REACQUIRE);
        in.line = IR_LINE_VISIBLE;
        in.guidance_valid = true;
        in.line_error = error;
        in.stationary = false;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE && out.start_scanner);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        input.now_ms = t;
        motion_control_step(&controller, &out.command, &input, &motion);
        assert(motion.status == RD_MOTION_IDLE);
        in.motion_status = motion.status;
        in.motion_action = motion.action_id;
        in.motion_stopped = motion.status == RD_MOTION_FAILED;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_steering(&out.command) == ref_initial_steer(error));
        enter_line_recovery(&m, &in, &t);
        out = step(&m, &in, t += RECOVERY_MS);
        assert(m.state == MISSION_STOPPED && out.line_lost);
    }
}

static void test_recovery_reverses_through_motion(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    MissionOutputs out = enter_line_recovery(&m, &in, &t);
    MotionController controller;
    motion_control_reset(&controller);
    MotionOutput motion;
    unsigned previous_action = 0u;
    for (unsigned leg = 0u; leg < 8u; ++leg)
    {
        in.stationary = true;
        out = step(&m, &in, t += MISSION_STATIONARY_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        assert(motion_cmd_speed(&out.command) == RECOVERY_SPEED_MMPS);
        assert(motion_cmd_action(&out.command) != previous_action);
        assert((leg & 1u) == 0u ? motion_cmd_target(&out.command) > 0 :
            motion_cmd_target(&out.command) < 0);
        previous_action = motion_cmd_action(&out.command);
        MotionInput input = {.now_ms = t,
            .count = {in.wheel_count[0], in.wheel_count[1]}};
        motion_control_step(&controller, &out.command, &input, &motion);
        assert(motion.status == RD_MOTION_RUNNING);
        in.motion_status = motion.status;
        in.motion_action = motion.action_id;
        in.stationary = false;
        float heading = (leg & 1u) == 0u ?
            RECOVERY_TARGET_DEG - RECOVERY_TARGET_TOL_DEG + 1.0f :
            -RECOVERY_TARGET_DEG + RECOVERY_TARGET_TOL_DEG - 1.0f;
        float arc = heading / 57.2957795f * TRACK_WIDTH_MM *
            ENC_COUNTS_PER_MM * 0.5f;
        in.wheel_count[0] = enc_advance(0u, -(int32_t)lroundf(arc));
        in.wheel_count[1] = enc_advance(0u, (int32_t)lroundf(arc));
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        input = (MotionInput){.now_ms = t,
            .count = {in.wheel_count[0], in.wheel_count[1]}};
        motion_control_step(&controller, &out.command, &input, &motion);
        assert(motion.status == RD_MOTION_IDLE);
        assert(motion.duty[0] == 0.0f && motion.duty[1] == 0.0f);
        in.motion_status = motion.status;
        in.motion_action = motion.action_id;
        in.motion_stopped = motion.status == RD_MOTION_FAILED;
        heading = (leg & 1u) == 0u ?
            RECOVERY_TARGET_DEG + 10.0f : -RECOVERY_TARGET_DEG - 10.0f;
        arc = heading / 57.2957795f * TRACK_WIDTH_MM *
            ENC_COUNTS_PER_MM * 0.5f;
        in.wheel_count[0] = enc_advance(0u, -(int32_t)lroundf(arc));
        in.wheel_count[1] = enc_advance(0u, (int32_t)lroundf(arc));
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_REACQUIRE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    }
    out = step(&m, &in, m.recovery.started_ms + RECOVERY_MS);
    assert(m.state == MISSION_STOPPED && out.line_lost);
    puts("INT-62: eight alternating sweep legs and bounded deadline PASS");
}

static void test_recovery_sweep_targets(void)
{
    for (unsigned scenario = 0u; scenario < 6u; ++scenario)
    {
        recovery_control_t control;
        recovery_input_t in = {.now_ms = 100u, .sampled_ms = 100u,
            .functional = true, .stationary = true};
        recovery_output_t out;
        recovery_control_begin(&control, &in);
        recovery_control_step(&control, &in, &out);
        assert(out.phase == RECOVERY_SEARCH && out.turn_degrees == 70);
        if (scenario == 0u)
        {
            in.motion_failed = true;
        }
        else if (scenario == 1u)
        {
            in.hazard = true;
        }
        else if (scenario == 2u)
        {
            in.cancel = true;
        }
        else if (scenario == 3u)
        {
            in.now_ms += IR_MAX_GAP_MS + 1u;
        }
        else if (scenario == 4u)
        {
            in.functional = false;
        }
        else
        {
            float arc = (RECOVERY_TARGET_DEG + RECOVERY_OVERSHOOT_DEG +
                1.0f) / 57.2957795f * TRACK_WIDTH_MM *
                ENC_COUNTS_PER_MM * 0.5f;
            in.count[0] = enc_advance(0u, -(int32_t)lroundf(arc));
            in.count[1] = enc_advance(0u, (int32_t)lroundf(arc));
        }
        recovery_control_step(&control, &in, &out);
        assert(out.phase == RECOVERY_FAILED && !out.turn);
    }
}

/* ---- Bypass ------------------------------------------------------------ */
static MissionOutputs bstep(Mission *m, MissionInputs *in, uint32_t *t)
{
    *t += 10u;
    return step(m, in, *t);
}

static void into_scanning(Mission *m, MissionInputs *in, uint32_t *t,
    uint32_t left_mm, uint32_t right_mm)
{
    *t = to_follow(m, in);
    in->front_valid = true;
    in->front_age_ms = 0u;
    in->front_mm = MISSION_OBSTACLE_STOP_MM;
    MissionOutputs out = bstep(m, in, t);
    assert(m->state == MISSION_SCAN);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = bstep(m, in, t);
    assert(!out.scan_request);
    in->stationary = true;
    out = bstep(m, in, t);
    assert(out.scan_request && m->scan_phase == 1u);
    in->scan_id = out.scan_id;
    in->scan_state = S5_SCAN_DONE;
    in->profile_valid = true;
    in->profile_closest_mm = 300u;
    in->profile_planning_mm = S5_EXPECTED_MAX_WIDTH_MM;
    in->profile_forward_mm = 150;
    in->left_clearance_mm = left_mm;
    in->right_clearance_mm = right_mm;
    in->front_mm = 1000u;
}

/* Simulate completed wheel travel, not just an unsubstantiated DONE flag. */
static void complete_action(MissionInputs *in, const MissionOutputs *out)
{
    float target = motion_cmd_target(&out->command);
    int32_t left = 0, right = 0;
    if (motion_cmd_mode(&out->command) == MOTION_MODE_TURN)
    {
        right = (int32_t)lroundf(target * 0.0174532925f *
            TRACK_WIDTH_MM * 0.5f * ENC_COUNTS_PER_MM);
        left = -right;
    }
    else if (motion_cmd_mode(&out->command) == MOTION_MODE_DISTANCE)
    {
        left = right = (int32_t)lroundf(target * ENC_COUNTS_PER_MM);
    }
    in->wheel_count[0] += (uint32_t)left;
    in->wheel_count[1] += (uint32_t)right;
    in->motion_action = motion_cmd_action(&out->command);
    in->motion_status = RD_MOTION_DONE;
}

static void fresh_front(MissionInputs *in, uint32_t now)
{
    in->front_valid = true;
    in->front_mm = 1000u;
    in->front_sampled_ms = now;
    in->front_sequence++;
    in->front_wheel_count[0] = in->wheel_count[0];
    in->front_wheel_count[1] = in->wheel_count[1];
}

static void run_bypass_sequence(uint32_t left_mm, uint32_t right_mm)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    into_scanning(&m, &in, &t, left_mm, right_mm);
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.bypass.left_side == (left_mm >= right_mm));
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    bool depth_seen = false, reacquire_seen = false;
    uint32_t routes = 0u;
    for (unsigned i = 0u; i < 500u && m.state != MISSION_FOLLOW_LINE; ++i)
    {
        assert(m.state == MISSION_BYPASS || m.state == MISSION_REACQUIRE);
        routes |= 1u << m.bypass.route;
        reacquire_seen |= m.state == MISSION_REACQUIRE;
        if (motion_cmd_mode(&out.command) == MOTION_MODE_TURN ||
            motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE)
        {
            complete_action(&in, &out);
        }
        else
        {
            in.motion_status = RD_MOTION_IDLE;
        }
        fresh_front(&in, t + 10u);
        in.side_valid = true;
        in.side_mm = m.bypass.forward_q8 / 256 > 250 ? 700u : 400u;
        in.side_sampled_ms = t + 10u;
        in.side_sequence++;
        out = bstep(&m, &in, &t);
        if (out.event_kind == MQTT_EVENT_OBSTACLE)
        {
            /* First edge at ~300 mm, mount +100 mm, ray +376 mm,
               profile front at 150 mm; allow 2 mm encoder quantization. */
            assert(fabsf((float)out.event_arg2 - 626.0f) <= 2.0f);
            depth_seen = true;
        }
    }
    assert(m.state == MISSION_FOLLOW_LINE && depth_seen && reacquire_seen);
    assert((routes & (1u << BYPASS_ROUTE_OFFSET)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_PASS)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_REAR)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_RETURN)) != 0u);
    assert(fabsf(m.bypass.left_q8 / 256.0f) <= BYPASS_RETURN_LATERAL_MM);
}

static void test_bypass_long_encoder_travel(void)
{
    bypass_control_t control;
    bypass_input_t in = {.now_ms = 100u, .stationary = true, .functional = true};
    ObstacleProfile profile = {.valid = true, .closest_mm = 500u,
        .left_clearance_mm = 800u, .right_clearance_mm = 400u};
    uint8_t action = 1u;
    assert(bypass_control_begin(&control, &in, &profile, 200u, &action));
    const uint32_t left_counts[] = {10000u, 20000u, 32760u};
    for (unsigned i = 0u; i < 3u; ++i)
    {
        in.count[0] = left_counts[i];
        in.count[1] = left_counts[i] + 20u;
        in.now_ms += 20u;
        in.stationary = false;
        bypass_output_t out;
        bypass_control_step(&control, &in, &action, &out);
        assert(out.route == BYPASS_ROUTE_OUT);
        assert(control.heading_cdeg > 0 && control.heading_cdeg < 100);
    }
    puts("INT-67: differential heading survives long common wheel travel PASS");
}

static void test_bypass_full_sequence(void)
{
    run_bypass_sequence(800u, 400u);
    run_bypass_sequence(400u, 900u);
}

static void test_bypass_failures(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    into_scanning(&m, &in, &t, MISSION_BYPASS_MIN_CLEAR_MM - 1u,
        MISSION_BYPASS_MIN_CLEAR_MM - 1u);
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* Wrong scan IDs cannot release movement; failed scans retry boundedly. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    in.scan_id = (uint8_t)(m.scan_id + 1u);
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_SCAN && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.scan_state = S5_SCAN_FAILED;
    for (unsigned i = 0u; i <= MISSION_SCAN_RETRIES; ++i)
    {
        in.scan_id = m.scan_id;
        out = bstep(&m, &in, &t);
    }
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* DONE without encoder evidence is rejected. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = bstep(&m, &in, &t);
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_DONE;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* A close front obstacle after the turn cannot authorize an offset. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = bstep(&m, &in, &t);
    complete_action(&in, &out);
    fresh_front(&in, t + 10u);
    out = bstep(&m, &in, &t);
    assert(m.bypass.route == BYPASS_ROUTE_OFFSET);
    fresh_front(&in, t + 10u);
    in.front_mm = 10u;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* Cached front samples cannot keep a drive alive. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = bstep(&m, &in, &t);
    complete_action(&in, &out);
    fresh_front(&in, t + 10u);
    bstep(&m, &in, &t);
    fresh_front(&in, t + 10u);
    out = bstep(&m, &in, &t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    in.motion_status = RD_MOTION_RUNNING;
    out = step(&m, &in, t += BYPASS_FRONT_MAX_AGE_MS + 1u);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = bstep(&m, &in, &t);
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_FAILED;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = step(&m, &in, t += MISSION_BYPASS_TIMEOUT_MS);
    assert(m.state == MISSION_STOPPED && out.stop_now);
}

static void test_straight_exit_guidance(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    assert(mission_queue_direction(&m, BARCODE_STRAIGHT));
    cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = -127;
    MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.state == MISSION_NAVIGATE);
    assert(motion_cmd_steering(&out.command) == -127);
    in.hump_active = true;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(motion_cmd_steering(&out.command) == -127);
    assert(motion_cmd_speed(&out.command) == 28u);
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(motion_cmd_steering(&out.command) == 0);
    assert(m.line_steer_q == 0);
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 127;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(motion_cmd_steering(&out.command) == 127);
    in.guidance_valid = false;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    puts("INT-91: angled straight exit, broad reset, hump and invalid guidance PASS");
}

static void test_digital_tracking(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    /* Mission publishes digital targets; Motion owns the applied steering slew. */
    const int errors[] = {-127, -127, -127, -127,
        127, 127, 127, 127, 127, 127, 127, 0, 0, 0, -127, 0, 127, 0};
    const int expected[] = {-127, -127, -127, -127,
        127, 127, 127, 127, 127, 127, 127, 0, 0, 0, -127, 0, 127, 0};
    const unsigned expected_speed[] = {56, 56, 56, 56,
        56, 56, 56, 56, 56, 56, 56, 90, 90, 90, 56, 90, 56, 90};
    for (unsigned i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
    {
        in.line_error = (int8_t)errors[i];
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_steering(&out.command) == expected[i]);
        assert(motion_cmd_speed(&out.command) == expected_speed[i]);
        assert(out.command.sampled_ms == in.line_sampled_ms);
        /* A second Mission release with the same acquisition changes nothing. */
        in.now_ms = t + FOLLOW_PERIOD_MS;
        mission_step(&m, &in, &out);
        assert(motion_cmd_steering(&out.command) == expected[i]);
        assert(motion_cmd_speed(&out.command) == expected_speed[i]);
    }
    /* A just-entered state must also accept a still-fresh older sample. */
    Mission recent;
    MissionInputs recent_in = base(100u);
    uint32_t recent_t = to_follow(&recent, &recent_in);
    recent_in.line_error = 127;
    recent_in.now_ms = recent_t;
    recent_in.line_sampled_ms = recent_t - IR_SAMPLE_MS;
    MissionOutputs recent_out;
    mission_step(&recent, &recent_in, &recent_out);
    assert(motion_cmd_steering(&recent_out.command) == 127);
    /* Wrapping acquisition times still identify a new sample. */
    m.line_steer_sampled_ms = UINT32_MAX - 9u;
    in.line_error = -127;
    MissionOutputs wrapped = step(&m, &in, 10u);
    assert(motion_cmd_steering(&wrapped.command) == -127);

    in.stop_down = true;
    MissionOutputs out = step(&m, &in, t + 1u);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(m.line_steer_q == 0);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
}

static void test_curve_speed_continuity(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    const int errors[] = {-127, -127, 0, 127, 127, 0, -127};
    for (unsigned i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
    {
        in.line_error = (int8_t)errors[i];
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_speed(&out.command) ==
            ref_speed(motion_cmd_steering(&out.command)));
        in.now_ms = t + FOLLOW_PERIOD_MS;
        mission_step(&m, &in, &out);
        assert(motion_cmd_speed(&out.command) ==
            ref_speed(motion_cmd_steering(&out.command)));
    }
    in.hump_active = true;
    MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(motion_cmd_speed(&out.command) ==
        (unsigned)floorf(MISSION_TRACK_SPEED_MMPS *
            1.0f / (1.0f + 0.60f * abs(motion_cmd_steering(&out.command)) / 127.0f) * 0.5f));
    in.stop_down = true;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(out.stop_now && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    puts("INT-89: curve speed follows steering; duplicate acquisitions, hump and stop PASS");
}

static void test_missing_echo_policy(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.front_valid = false;
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.state == MISSION_FOLLOW_LINE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);

    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    out = bstep(&m, &in, &t);
    complete_action(&in, &out);
    fresh_front(&in, t + 10u);
    out = bstep(&m, &in, &t);
    assert(m.bypass.route == BYPASS_ROUTE_OFFSET);
    fresh_front(&in, t + 10u);
    out = bstep(&m, &in, &t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    in.motion_status = RD_MOTION_RUNNING;
    in.front_valid = false;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    puts("INT-24 no echo: PASS (tracking report-only; bypass offset stops)");
}

static void test_stop_cancels_every_phase(void)
{
    _Static_assert(MISSION_STATE_COUNT == 7u, "seven states, no COMPLETE");
    for (unsigned state = 0u; state < MISSION_STATE_COUNT; ++state)
    {
        Mission m;
        MissionInputs in = base(100u);
        mission_init(&m, 100u);
        m.state = (MissionState)state;
        m.held = BARCODE_LEFT;
        in.stop_down = true;
        MissionOutputs out = step(&m, &in, 110u);
        assert(m.state == MISSION_STOPPED);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        if (state != MISSION_STOPPED)
        {
            assert(out.scan_cancel && out.stop_scanner);
            assert(m.held == BARCODE_UNMAPPED);
        }
    }
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.motion_stopped = true; /* independent Motion expiry latch */
    MissionOutputs out = step(&m, &in, t + 10u);
    assert(m.state == MISSION_STOPPED);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
}

static void test_run_network_loss_and_hump_speed(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.start_blocked = true; /* connectivity lost after accepted local start */
    MissionOutputs out = step(&m, &in, t += 20u);
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_now);
    unsigned normal = motion_cmd_speed(&out.command);
    in.hump_active = true;
    out = step(&m, &in, t += 20u);
    assert(motion_cmd_speed(&out.command) == normal / 2u);
    in.hump_active = false;
    out = step(&m, &in, t += 20u);
    assert(motion_cmd_speed(&out.command) == normal);
}

static void test_queued_direction_on_first_black(void)
{
    for (unsigned source = 0u; source < 2u; ++source)
    {
        for (uint8_t action = BARCODE_LEFT; action <= BARCODE_U_TURN; ++action)
        {
            Mission m;
            MissionInputs in = base(100u);
            uint32_t t = to_follow(&m, &in);
            if (source == 0u)
            {
                assert(mission_queue_direction(&m, action));
            }
            else
            {
                in.have_barcode = true;
                in.barcode_action = action;
                step(&m, &in, t += IR_SAMPLE_MS);
                in.have_barcode = false;
            }
            in.line = IR_LINE_BROAD_BLACK;
            in.guidance_valid = false;
            /* No encoder travel and no black-exit sample before navigation. */
            MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.state == MISSION_NAVIGATE);
            assert(m.held == BARCODE_UNMAPPED && !m.manual_held);
            assert(motion_cmd_mode(&out.command) ==
                (action == BARCODE_U_TURN ? MOTION_MODE_STOP : MOTION_MODE_FOLLOW));
            if (action == BARCODE_LEFT || action == BARCODE_RIGHT)
            {
                assert(m.phase == JUNCTION_PHASE_ADVANCE);
                assert(motion_cmd_steering(&out.command) == 0);
                assert(motion_cmd_speed(&out.command) == 90u);
            }
            uint8_t id = m.current_action;
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.state == MISSION_NAVIGATE && m.current_action == id);
            assert(m.held == BARCODE_UNMAPPED && out.event_kind == 0u);
        }
    }
    puts("queued directions: first both-black sample, zero travel, consumed once PASS");
}

static void test_manual_junction(void)
{
    for (uint8_t action = BARCODE_LEFT; action <= BARCODE_U_TURN; ++action)
    {
        Mission m;
        MissionInputs in = base(100u);
        mission_init(&m, 100u);
        assert(!mission_queue_direction(&m, action));
        uint32_t t = to_follow(&m, &in);
        assert(!mission_queue_direction(&m, 0u));
        assert(!mission_queue_direction(&m, 5u));
        m.in_broad = 1u;
        assert(!mission_queue_direction(&m, action));
        m.in_broad = 0u;
        m.held = BARCODE_RIGHT;
        assert(mission_queue_direction(&m, action));
        assert(!mission_queue_direction(&m, action));
        in.have_barcode = true;
        in.barcode_action = BARCODE_LEFT;
        MissionOutputs out = step(&m, &in, t += 10u);
        assert(m.held == action && m.manual_held);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        in.have_barcode = false;
        cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
        out = step(&m, &in, t += 10u);
        assert(m.state == MISSION_NAVIGATE);
        assert(m.held == BARCODE_UNMAPPED && !m.manual_held);
        assert(!mission_queue_direction(&m, action));
        if (action == BARCODE_LEFT || action == BARCODE_RIGHT)
        {
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
            assert(motion_cmd_target(&out.command) ==
                (action == BARCODE_LEFT ? 360.0f : -360.0f));
        }
        t = to_follow(&m, &in);
        assert(mission_queue_direction(&m, action));
        in.stop_down = true;
        out = step(&m, &in, t += 10u);
        assert(m.state == MISSION_STOPPED && !m.manual_held &&
            m.held == BARCODE_UNMAPPED);
    }
    puts("INT-36: manual direction priority, junction consumption and stop cancellation PASS");
}

static void test_junction_divergent_wheel_counts(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    in.wheel_count[0] = 1000u;
    in.wheel_count[1] = 33767u;
    step(&m, &in, t += 10u);
    in.wheel_count[0] += 1u;
    in.wheel_count[1] += 2u;
    step(&m, &in, t += 20u);
    assert(!m.junction_qualified);
    in.wheel_count[0] += MISSION_JUNCTION_MIN_COUNTS;
    in.wheel_count[1] += MISSION_JUNCTION_MIN_COUNTS;
    step(&m, &in, t += 20u);
    assert(m.junction_qualified);
}

/* Encoder-based speed ramps never replace IR turn completion. */
static void test_junction_speed_ramps(void)
{
    for (unsigned turn = 0; turn < 3; ++turn)
    {
        Mission m;
        MissionInputs in = base(100u);
        uint32_t t = to_follow(&m, &in);
        barcode_action_t actions[] = {BARCODE_STRAIGHT, BARCODE_LEFT,
            BARCODE_RIGHT, BARCODE_U_TURN};
        assert(mission_queue_direction(&m, actions[turn]));
        cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_NAVIGATE);
        if (turn == 3)
        {
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
            in.stationary = true;
            out = step(&m, &in, t += IR_SAMPLE_MS);
            out = step(&m, &in, t += MISSION_UTURN_PAUSE_MS);
        }
        assert(motion_cmd_speed(&out.command) == (turn == 0 ? 90u : 37u));
        uint8_t action_id = motion_cmd_action(&out.command);
        /* Time alone must not increase the request. */
        out = step(&m, &in, t += 200u);
        assert(motion_cmd_speed(&out.command) == (turn == 0 ? 90u : 37u));
        /* Exercise modular encoder arithmetic across zero. */
        m.broad_origin[0] = m.broad_origin[1] = ENC_COUNT_MASK - 3u;
        in.wheel_count[0] = in.wheel_count[1] = ENC_COUNT_MASK - 3u;
        float arc = turn == 0 ? 0.0f : TRACK_WIDTH_MM *
            90.0f * 0.01745329252f;
        float length = turn == 0 ? MISSION_STRAIGHT_MM : arc;
        unsigned prior = turn == 0 ? 90u : 37u;
        for (unsigned i = 1; i <= 4; ++i)
        {
            int counts = i == 4 ?
                (int)ceilf(length * ENC_COUNTS_PER_MM) :
                (int)floorf(length * i * 0.25f * ENC_COUNTS_PER_MM);
            in.wheel_count[0] = enc_advance(m.broad_origin[0],
                turn == 0 || turn == 2 ? counts : 0);
            in.wheel_count[1] = enc_advance(m.broad_origin[1],
                turn == 2 ? 0 : counts);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            unsigned speed = motion_cmd_speed(&out.command);
            if (m.state == MISSION_FOLLOW_LINE)
            {
                assert(speed == (unsigned)MISSION_TRACK_SPEED_MMPS);
            }
            else
            {
                assert(speed >= prior && speed <= (turn == 0 ? 90u : 75u));
            }
            prior = speed;
        }
        assert(prior > 37u);
        if (turn == 0)
        {
            assert(m.state == MISSION_FOLLOW_LINE &&
                prior == (unsigned)MISSION_TRACK_SPEED_MMPS);
        }
        if (turn != 0)
        {
            assert(m.phase == JUNCTION_PHASE_TURN);
            assert(motion_cmd_action(&out.command) == action_id);
            assert(motion_cmd_speed(&out.command) == 75u);
        }
        in.stop_down = true;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_STOPPED && out.stop_now);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    }
    puts("junction ramps: PASS (straight/left/right, swing progress, wrap, stop)");
}

static void test_navigation_wheels_and_turn_ir(void)
{
    const uint8_t actions[] = {BARCODE_LEFT, BARCODE_RIGHT, BARCODE_U_TURN};
    for (unsigned direction = 0; direction < 3u; ++direction)
    {
        Mission m;
        MissionInputs in = base(direction == 2u ? UINT32_MAX - 100u : 100u);
        uint32_t t = to_follow(&m, &in);
        assert(mission_queue_direction(&m, actions[direction]));
        in.line = IR_LINE_BROAD_BLACK;
        in.left_black = in.right_black = true;
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_NAVIGATE);
        if (direction == 2u)
        {
            in.stationary = true;
            step(&m, &in, t += IR_SAMPLE_MS);
            out = step(&m, &in, t += MISSION_UTURN_PAUSE_MS);
        }
        if (direction < 2u)
        {
            advance_to_turn(&m, &in, &t);
            out = step(&m, &in, t += IR_SAMPLE_MS);
        }
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        assert(motion_cmd_steering(&out.command) ==
            (direction == 2u ? MOTION_TURN_PIVOT : MOTION_TURN_SWING));
        MotionController controller;
        motion_control_reset(&controller);
        MotionInput input = {.now_ms = t};
        MotionOutput motor;
        motion_control_step(&controller, &out.command, &input, &motor);
        if (direction == 0u)
        {
            assert(motor.duty[0] == 0.0f && motor.duty[1] > 0.0f);
        }
        else if (direction == 1u)
        {
            assert(motor.duty[0] > 0.0f && motor.duty[1] == 0.0f);
        }
        else
        {
            assert(motor.duty[0] > 0.0f && motor.duty[1] < 0.0f);
        }
        in.stationary = false;
        in.motion_status = motor.status;
        in.motion_action = motor.action_id;
        bool *turning_black = direction == 0u ? &in.left_black : &in.right_black;
        bool *other_black = direction == 0u ? &in.right_black : &in.left_black;
        /* Starting black is ignored, including the first black-to-white exit. */
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.turn_line_phase == 0u);
        *turning_black = false;
        in.line = IR_LINE_VISIBLE;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.turn_line_phase == 1u && m.state == MISSION_NAVIGATE);
        *other_black = false;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        *other_black = true;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.turn_line_phase == 1u); /* opposite sensor cannot finish */
        *turning_black = true;
        in.now_ms = t + FOLLOW_PERIOD_MS;
        mission_step(&m, &in, &out);
        assert(m.turn_line_phase == 1u); /* duplicate acquisition */
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.turn_line_phase == 2u);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
        *turning_black = false;
        in.now_ms = t + FOLLOW_PERIOD_MS;
        mission_step(&m, &in, &out);
        assert(m.state == MISSION_NAVIGATE);
        assert(out.command.sampled_ms == in.line_sampled_ms);
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_NAVIGATE && m.phase == JUNCTION_PHASE_BRAKE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        /* Simulate a faster Mission release before Motion reads STOP. */
        out = step(&m, &in, t += FOLLOW_PERIOD_MS);
        assert(m.state == MISSION_NAVIGATE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        input.now_ms = t;
        motion_control_step(&controller, &out.command, &input, &motor);
        assert(motor.duty[0] == 0.0f && motor.duty[1] == 0.0f);
        in.motion_status = motor.status;
        in.motion_action = motor.action_id;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        in.guidance_valid = true;
        in.left_black = in.right_black = false;
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        input.now_ms = t;
        motion_control_step(&controller, &out.command, &input, &motor);
        assert(motor.duty[0] > 0.0f && motor.duty[1] > 0.0f);
        assert(!out.stop_now);
        if (direction == 2u)
        {
            in.line_error = -127;
            in.left_black = true;
            for (unsigned sample = 0; sample < 8u; ++sample)
            {
                out = step(&m, &in, t += IR_SAMPLE_MS);
                input.now_ms = t;
                motion_control_step(&controller, &out.command, &input, &motor);
                assert(controller.follow_steer_q ==
                    -(int)(sample == 7u ? 127u : 16u * (sample + 1u)));
                assert(motor.duty[0] >= 0.0f && motor.duty[1] > 0.0f);
                if (sample == 7u)
                {
                    assert(motion_cmd_steering(&out.command) == -127);
                    assert(motion_cmd_speed(&out.command) == 56u);
                    assert(motor.duty[0] == 0.0f);
                }
            }
        }
    }
    for (unsigned direction = 0; direction < 3u; ++direction)
    {
        for (unsigned cause = 0; cause < 9u; ++cause)
        {
            Mission m;
            MissionInputs in = base(direction == 2u ? UINT32_MAX - 100u : 100u);
            uint32_t t = to_follow(&m, &in);
            assert(mission_queue_direction(&m, actions[direction]));
            in.line = IR_LINE_BROAD_BLACK;
            in.left_black = in.right_black = true;
            step(&m, &in, t += IR_SAMPLE_MS);
            if (direction == 2u)
            {
                in.stationary = true;
                step(&m, &in, t += IR_SAMPLE_MS);
                step(&m, &in, t += MISSION_UTURN_PAUSE_MS);
            }
            if (direction < 2u)
            {
                advance_to_turn(&m, &in, &t);
            }
            in.stationary = false;
            if (cause == 0u)
            {
                in.usable_line = false;
            }
            else if (cause == 1u)
            {
                in.now_ms = t + IR_MAX_GAP_MS + 1u;
            }
            else if (cause == 2u || cause == 7u)
            {
                in.motion_action = m.current_action;
                in.motion_status = cause == 2u ? RD_MOTION_DONE : RD_MOTION_FAILED;
            }
            else if (cause == 3u)
            {
                m.turn_line_phase = 3u;
            }
            else if (cause == 4u)
            {
                in.stop_down = true;
            }
            else if (cause == 5u)
            {
                in.front_valid = true;
                in.front_mm = MISSION_OBSTACLE_STOP_MM;
            }
            else if (cause == 8u)
            {
                m.phase = 7u;
            }
            else
            {
                in.now_ms = m.state_since_ms + MOTION_ACTION_TIMEOUT_MS;
                in.line_sampled_ms = in.now_ms;
            }
            MissionOutputs out;
            mission_step(&m, &in, &out);
            assert(m.state == MISSION_STOPPED && out.stop_now);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        }
    }
    puts("INT-84: L/R/U wheel signs, turning-side fresh exit/black/white, FOLLOW handoff and guards PASS");
}

static void test_turn_advance_70mm(void)
{
    for (unsigned direction = 0; direction < 2u; ++direction)
    {
        for (unsigned scenario = 0; scenario < 8u; ++scenario)
        {
            Mission m;
            MissionInputs in = base(100u);
            uint32_t t = to_follow(&m, &in);
            assert(mission_queue_direction(&m,
                direction == 0u ? BARCODE_LEFT : BARCODE_RIGHT));
            in.wheel_count[0] = ENC_COUNT_MASK - 200u;
            in.wheel_count[1] = 1000u;
            in.line = IR_LINE_BROAD_BLACK;
            in.guidance_valid = false;
            in.left_black = in.right_black = true;
            MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.phase == JUNCTION_PHASE_ADVANCE);
            assert(m.held == BARCODE_UNMAPPED);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
            assert(motion_cmd_speed(&out.command) == 90u);
            assert(motion_cmd_steering(&out.command) == 0);
            uint32_t origin[2] = {in.wheel_count[0], in.wheel_count[1]};
            /* Neither elapsed time nor backing up may trigger the turn. */
            out = step(&m, &in, t += 100u);
            assert(m.phase == JUNCTION_PHASE_ADVANCE);
            in.wheel_count[0] = enc_advance(origin[0], -20);
            in.wheel_count[1] = enc_advance(origin[1], -20);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.phase == JUNCTION_PHASE_ADVANCE);
            int threshold = (int)ceilf(70.0f * ENC_COUNTS_PER_MM);
            in.wheel_count[0] = enc_advance(origin[0], threshold - 1);
            in.wheel_count[1] = enc_advance(origin[1], threshold - 1);
            out = step(&m, &in, t += IR_SAMPLE_MS);
            assert(m.phase == JUNCTION_PHASE_ADVANCE);
            if (scenario == 0u || scenario == 7u)
            {
                in.left_black = in.right_black = scenario == 7u;
                in.wheel_count[0] = enc_advance(origin[0], threshold);
                in.wheel_count[1] = enc_advance(origin[1], threshold);
                out = step(&m, &in, t += IR_SAMPLE_MS);
                assert(m.phase == JUNCTION_PHASE_TURN);
                assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
                assert(motion_cmd_speed(&out.command) == 37u);
                assert(motion_cmd_steering(&out.command) == MOTION_TURN_SWING);
                assert(m.turn_line_phase == (scenario == 7u ? 0u : 1u));
                assert(m.broad_origin[0] == in.wheel_count[0]);
                assert(m.broad_origin[1] == in.wheel_count[1]);
            }
            else
            {
                if (scenario == 1u)
                {
                    in.stop_down = true;
                }
                else if (scenario == 2u)
                {
                    in.front_valid = true;
                    in.front_mm = MISSION_OBSTACLE_STOP_MM;
                }
                else if (scenario == 3u)
                {
                    in.usable_line = false;
                }
                else if (scenario == 4u)
                {
                    in.now_ms = t + IR_MAX_GAP_MS + 1u;
                }
                else if (scenario == 5u)
                {
                    in.now_ms = m.state_since_ms + MOTION_ACTION_TIMEOUT_MS;
                    in.line_sampled_ms = in.now_ms;
                }
                else
                {
                    in.motion_action = 1u;
                    in.motion_status = RD_MOTION_FAILED;
                }
                mission_step(&m, &in, &out);
                assert(m.state == MISSION_STOPPED && out.stop_now);
                assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
            }
        }
    }
    puts("INT-85: 70 mm L/R approach, signed/wrapped counts, IR reset and guards PASS");
}

int main(void)
{
    test_turn_advance_70mm();
    test_navigation_wheels_and_turn_ir();
    test_junction_speed_ramps();
    test_junction_divergent_wheel_counts();
    test_stop_cancels_every_phase();
    test_queued_direction_on_first_black();
    test_run_network_loss_and_hump_speed();
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_digital_tracking();
    test_curve_speed_continuity();
    test_missing_echo_policy();
    test_both_black_forward();
    test_barcode_hold_and_junction();
    test_manual_junction();
    test_crossing_never_completes_mission();
    test_straight_junction_segment();
    test_straight_exit_guidance();
    test_line_loss_qualification();
    test_line_loss_recovery_is_bounded();
    test_recovery_sweep_targets();
    test_recovery_reverses_through_motion();
    test_bypass_full_sequence();
    test_bypass_long_encoder_travel();
    test_bypass_failures();
    puts("mission: PASS");
    return 0;
}
