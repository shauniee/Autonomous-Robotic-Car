/* Host test for app_program/mission/mission_logic.[ch] (issue #36). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/common/robot_time_math.h"

static MissionInputs base(uint32_t now)
{
    return (MissionInputs){.now_ms = now, .inputs_ok = true, .calibration_valid = true, .guidance_valid = true,
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

/* Line PID reference (float, from the spec): steering in Q7 for a two-sensor
   error in Q7, with the integral/derivative history held by the caller. */
typedef struct { float integral, prev, out; uint32_t ms; int primed; } RefPid;

static int ref_pid(RefPid *r, int error_q, uint32_t sampled_ms)
{
    float e = (float)error_q / IR_SCALE_ONE, d = 0.0f, dt = IR_SAMPLE_MS / 1000.0f;
    if (LINE_BINARY_RAMP)
    {
        int continuous = r->primed && sampled_ms != r->ms &&
            sampled_ms - r->ms <= IR_MAX_GAP_MS;
        if (continuous)
        {
            dt = (float)(sampled_ms - r->ms) / 1000.0f;
        }
        float target = LINE_BINARY_START;
        if (continuous && e * r->prev > 0.0f)
        {
            target = fminf(fabsf(r->prev) + LINE_BINARY_RATE * dt, MAX_STEER);
        }
        e = e > 0.0f ? target : (e < 0.0f ? -target : 0.0f);
        if (!continuous)
        {
            r->out = 0.0f;
        }
        float output = fminf(fmaxf(e, r->out - LINE_STEER_RATE * dt),
            r->out + LINE_STEER_RATE * dt);
        int q = (int)lroundf(output * IR_SCALE_ONE);
        r->prev = e; r->ms = sampled_ms; r->primed = 1;
        r->out = (float)q / IR_SCALE_ONE;
        return q;
    }
    if (r->primed && sampled_ms != r->ms) {
        dt = (float)(sampled_ms - r->ms) / 1000.0f;
        e = r->prev + (e - r->prev) * dt / (LINE_ERROR_TAU_MS / 1000.0f + dt);	/* low-pass */
        d = (e - r->prev) / dt;
        r->integral = fminf(fmaxf(r->integral + LINE_KI * e * dt, -LINE_I_LIMIT), LINE_I_LIMIT);
    }
    float s = LINE_KP * e + r->integral + LINE_KD * d;
    s = fminf(fmaxf(s, r->out - LINE_STEER_RATE * dt), r->out + LINE_STEER_RATE * dt);	/* rate limit */
    s = fminf(fmaxf(s, -MAX_STEER), MAX_STEER);
    int q = (int)lroundf(s * IR_SCALE_ONE);
    r->prev = e; r->ms = sampled_ms; r->primed = 1; r->out = (float)q / IR_SCALE_ONE;
    return q;
}

/* Follow speed for a steering command: slows toward MAX_STEER. */
static unsigned ref_speed(int steer_q)
{
    float share = fminf(fabsf((float)steer_q) / (MAX_STEER * IR_SCALE_ONE), 1.0f);
    return (unsigned)floorf(MISSION_FOLLOW_SPEED_MMPS * (1.0f - LINE_CURVE_SLOWDOWN * share));
}

/* First PID output after a reset: proportional, rate-limited from zero and
   limited, no history. */
static int ref_p(int error_q)
{
    RefPid r = {0};
    return ref_pid(&r, error_q, 0u);
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
    Mission m; MissionInputs in = base(100u);
    uint32_t t = 100u;
    mission_init(&m, t);
    step(&m, &in, t);
    assert(m.state == MISSION_READY);

    /* No mission-day letters: the press is refused and nothing is requested. */
    in.mapped = false; in.start_pressed = true; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(out.start_refused && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);

    /* A start-blocking fault refuses the press the same way. */
    in.mapped = true; in.start_blocked = true; t += 10u;
    out = step(&m, &in, t);
    assert(out.start_refused);

    /* A fault that appears while waiting for Motion cancels the pending start. */
    in.start_blocked = false; t += 10u;
    out = step(&m, &in, t);
    assert(out.start_refused);
    in.start_pressed = false; in.start_blocked = true; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY);
}

static void test_follow_and_stops(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line_error = IR_SCALE_ONE; t += 10u;			/* full right: proportional, limited to MAX_STEER */
    MissionOutputs out = step(&m, &in, t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW && motion_cmd_steering(&out.command) == ref_p(IR_SCALE_ONE));
    assert(motion_cmd_steering(&out.command) <= (int)lroundf(LINE_STEER_RATE * IR_SAMPLE_MS / 1000.0f * IR_SCALE_ONE));	/* a sudden full error ramps in */
    assert(out.command.sampled_ms == t && motion_cmd_speed(&out.command) == ref_speed(ref_p(IR_SCALE_ONE)));
    assert(ref_speed(0) == (unsigned)MISSION_FOLLOW_SPEED_MMPS && ref_speed(ref_p(IR_SCALE_ONE)) < ref_speed(0));	/* slower when steering */

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

static void cross_junction(Mission *m, MissionInputs *in, uint32_t *t, uint32_t length)
{
    in->line = IR_LINE_BROAD_BLACK;
    in->guidance_valid = false;
    in->wheel_count[0] = in->wheel_count[1] = 1000u;
    *t += 10u; step(m, in, *t);
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
    assert(m.state == MISSION_NAVIGATE && motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == 90.0f);
    uint8_t turn_id = motion_cmd_action(&out.command);
    assert(turn_id >= 2u && m.held == BARCODE_UNMAPPED);

    /* The next action starts only after DONE for the turn's ID. */
    in.motion_status = RD_MOTION_RUNNING; in.motion_action = turn_id; t += 10u;
    out = step(&m, &in, t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_action(&out.command) == turn_id);
    in.motion_status = RD_MOTION_DONE; t += 10u;
    out = step(&m, &in, t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE && motion_cmd_action(&out.command) != turn_id);
    uint8_t clear_id = motion_cmd_action(&out.command);
    in.motion_action = clear_id; in.motion_status = RD_MOTION_DONE; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_NAVIGATE && m.phase == JUNCTION_PHASE_CONFIRM);
    t += 20u; step(&m, &in, t);
    in.now_ms = t + 10u;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_NAVIGATE); /* repeated sample cannot confirm */
    for (unsigned i = 0u; i < MISSION_CONFIRM_SAMPLES; ++i)
    {
        t += 20u; step(&m, &in, t);
    }
    assert(m.state == MISSION_FOLLOW_LINE);

    /* Right and U-turn targets; an unused junction with nothing held carries straight on. */
    struct { barcode_action_t a; float deg; } turns[] = {{BARCODE_RIGHT, -90.0f}, {BARCODE_U_TURN, 180.0f}};
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
        in.line = IR_LINE_LOST;
        in.guidance_valid = false;
        MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_NAVIGATE && m.phase == JUNCTION_PHASE_STRAIGHT);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) == (unsigned)MISSION_FOLLOW_SPEED_MMPS);
        assert(motion_cmd_steering(&out.command) == 0);
        assert(m.held == BARCODE_UNMAPPED && !m.manual_held);
        uint32_t origin = in.wheel_count[0];
        unsigned counts = (unsigned)ceilf(MISSION_STRAIGHT_MM * ENC_COUNTS_PER_MM);
        if (scenario < 2u)
        {
            /* Visible samples cannot end the measured straight segment. */
            in.line = IR_LINE_VISIBLE;
            in.guidance_valid = true;
            in.line_error = 100;
            for (unsigned i = 0u; i < MISSION_CONFIRM_SAMPLES + 1u; ++i)
            {
                out = step(&m, &in, t += IR_SAMPLE_MS);
                assert(m.state == MISSION_NAVIGATE);
                assert(motion_cmd_steering(&out.command) == 0);
                assert(motion_cmd_speed(&out.command) == (unsigned)MISSION_FOLLOW_SPEED_MMPS);
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
                assert(motion_cmd_steering(&out.command) == ref_p(100));
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
            assert(motion_cmd_speed(&out.command) == (unsigned)(MISSION_FOLLOW_SPEED_MMPS * 0.5f));
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
    puts("INT-61: 25 mm full-speed straight exit, wrap, no visible gate and safety guards PASS");
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
        assert(motion_cmd_steering(&out.command) == ref_p(error));
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

/* The line PID matches the reference over a varying error, updates once per
   IR sample and restarts from zero history when the run segment changes. */
static void test_line_pid(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    static const int errors[] = {0, 10, 30, 30, 20, 5, -5, -20, -40, -40, -30, -10, 0, 0, 15, 127, 127, -127, 0};
    RefPid ref = {0};
    for (unsigned i = 0; i < sizeof errors / sizeof errors[0]; ++i) {
        t += 20u;					/* one IR sample */
        in.line_error = (int8_t)errors[i];
        MissionOutputs out = step(&m, &in, t);
        int expected = ref_pid(&ref, errors[i], t);
        int got = motion_cmd_steering(&out.command);
        assert(got >= expected - 1 && got <= expected + 1);	/* Q7 rounding */
        assert(got >= -(int)lroundf(MAX_STEER * IR_SCALE_ONE) && got <= (int)lroundf(MAX_STEER * IR_SCALE_ONE));
        assert(motion_cmd_speed(&out.command) == ref_speed(got));

        /* A Mission release between samples holds the output. */
        in.now_ms = t + 10u;
        mission_step(&m, &in, &out);
        assert(motion_cmd_steering(&out.command) == got);
    }
    /* Steer sign follows the line: more black on the left turns left. */
    Mission m2; MissionInputs in2 = base(100u);
    t = to_follow(&m2, &in2);
    in2.line_error = -40; t += 20u;
    MissionOutputs o2 = step(&m2, &in2, t);
    assert(motion_cmd_steering(&o2.command) < 0);
    in2.line_error = 40;
    for (int k = 0; k < 10; ++k) { t += 20u; o2 = step(&m2, &in2, t); }	/* filter lag: 200 ms */
    assert(motion_cmd_steering(&o2.command) > 0);
    /* Leaving FOLLOW discards the history. */
    in2.stop_down = true; t += 10u; step(&m2, &in2, t);
    assert(m2.state == MISSION_STOPPED && !m2.line_pid_primed && m2.line_integral_q == 0);
}

/* INT-24: persistent scaled history versus the prior float algorithm.
   Exercise both saturation signs, variable dt, timestamp wrap and invalid
   storage reset. One Q7 count is a software comparison bound, not tuning. */
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

static void test_binary_line_ramp(void)
{
    if (!LINE_BINARY_RAMP)
    {
        return;
    }
    Mission m, other;
    MissionInputs in = base(100u), other_in = base(100u);
    uint32_t t = to_follow(&m, &in);
    (void)to_follow(&other, &other_in);
    in.line_error = 127;
    other_in.line_error = 1;
    MissionOutputs out = step(&m, &in, t += IR_SAMPLE_MS);
    MissionOutputs other_out = step(&other, &other_in, t);
    int first = motion_cmd_steering(&out.command);
    assert(first > 0 && first <= (int)lroundf(LINE_BINARY_START * IR_SCALE_ONE));
    assert(first == motion_cmd_steering(&other_out.command));
    unsigned initial_speed = motion_cmd_speed(&out.command);
    int prior = first;
    for (unsigned i = 0u; i < 100u; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
        int steer = motion_cmd_steering(&out.command);
        assert(steer >= prior);
        assert(steer - prior <= (int)ceilf(LINE_STEER_RATE *
            IR_SAMPLE_MS / 1000.0f * IR_SCALE_ONE));
        assert(m.line_integral_q == 0);
        prior = steer;
    }
    assert(prior == (int)lroundf(MAX_STEER * IR_SCALE_ONE));
    assert(motion_cmd_speed(&out.command) < initial_speed);

    /* Reusing an acquisition cannot grow the target. */
    int16_t target = m.line_prev_error_q;
    in.now_ms = t + 1u;
    mission_step(&m, &in, &out);
    assert(m.line_prev_error_q == target);
    assert(motion_cmd_steering(&out.command) == prior);

    in.line_error = -127;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(fabsf((float)m.line_prev_error_q / 32767.0f +
        LINE_BINARY_START) <= 1.0f / 32767.0f);
    assert(abs(motion_cmd_steering(&out.command) - prior) <=
        (int)ceilf(LINE_STEER_RATE * IR_SAMPLE_MS / 1000.0f * IR_SCALE_ONE));
    for (unsigned i = 0u; i < 100u; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
    }
    assert(motion_cmd_steering(&out.command) == -(int)lroundf(MAX_STEER * IR_SCALE_ONE));

    /* A fresh sample after an acquisition gap restarts gently. */
    out = step(&m, &in, t += IR_MAX_GAP_MS + 1u);
    assert(motion_cmd_steering(&out.command) == -first);
    in.line_error = 0;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.line_prev_error_q == 0 && motion_cmd_steering(&out.command) == 0);

    /* Acquisition subtraction remains valid across timestamp wrap. */
    in.line_error = 127;
    t = UINT32_MAX - 10u;
    (void)step(&m, &in, t);
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.line_prev_error_q > (int16_t)lroundf(LINE_BINARY_START * 32767.0f));
    in.stop_down = true;
    (void)step(&m, &in, t + 1u);
    assert(m.state == MISSION_STOPPED && !m.line_pid_primed);
    assert(m.line_integral_q == 0 && m.line_steer_q == 0);
    puts("binary line ramp: PASS (buildup, reversal, slew, freshness, wrap, stop)");
}

static void test_line_history_storage(void)
{
    for (unsigned scenario = 0u; scenario < 3u; ++scenario)
    {
        Mission m;
        MissionInputs in = base(UINT32_MAX - 1000u);
        uint32_t t = to_follow(&m, &in);
        RefPid ref = {0};
        unsigned max_delta = 0u;
        for (unsigned i = 0u; i < 10000u; ++i)
        {
            int error = scenario == 0u ? 127 : -127;
            if (scenario == 2u)
            {
                error = (int)((i * 73u) % 255u) - 127;
            }
            t += 10u + i % 4u * 10u;
            in.line_error = (int8_t)error;
            MissionOutputs out = step(&m, &in, t);
            int expected = ref_pid(&ref, error, t);
            int got = motion_cmd_steering(&out.command);
            unsigned delta = (unsigned)(got > expected ? got - expected : expected - got);
            if (delta > max_delta)
            {
                max_delta = delta;
            }
            assert(delta <= 1u);
            assert(m.line_integral_q != INT16_MIN);
            assert(m.line_prev_error_q != INT16_MIN);
            assert(fabsf((float)m.line_integral_q / 32767.0f) <=
                LINE_I_LIMIT + 1.0f / 65534.0f);
        }
        printf("mission scaled history: scenario %u, 10000 samples, max Q7 delta %u\n",
            scenario, max_delta);
        m.line_integral_q = INT16_MIN;
        m.line_prev_error_q = INT16_MIN;
        in.line_error = 0;
        MissionOutputs out = step(&m, &in, t + 20u);
        assert(m.line_integral_q == 0 && m.line_prev_error_q == 0);
        assert(motion_cmd_steering(&out.command) == 0);
    }
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

static void test_thin_patch_does_not_consume_barcode(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.have_barcode = true;
    in.barcode_action = BARCODE_LEFT;
    step(&m, &in, t += 10u);
    in.have_barcode = false;
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    in.wheel_count[0] = in.wheel_count[1] = 1000u;
    MissionOutputs out = step(&m, &in, t += 10u);
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_RUNNING;
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.wheel_count[0] = in.wheel_count[1] = 1001u;
    step(&m, &in, t += 20u);
    in.wheel_count[0] = in.wheel_count[1] = 1200u; /* White travel cannot qualify the earlier thin patch. */
    in.motion_status = RD_MOTION_DONE;
    out = step(&m, &in, t += 20u);
    assert(m.state == MISSION_FOLLOW_LINE && m.held == BARCODE_LEFT);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
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
                (action == BARCODE_LEFT ? 90.0f : -90.0f));
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

int main(void)
{
    test_junction_divergent_wheel_counts();
    test_stop_cancels_every_phase();
    test_thin_patch_does_not_consume_barcode();
    test_run_network_loss_and_hump_speed();
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_line_pid();
    test_binary_line_ramp();
    test_line_history_storage();
    test_missing_echo_policy();
    test_both_black_forward();
    test_barcode_hold_and_junction();
    test_manual_junction();
    test_crossing_never_completes_mission();
    test_straight_junction_segment();
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
