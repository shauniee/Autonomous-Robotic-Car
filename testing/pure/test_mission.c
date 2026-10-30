/* Host test for app_program/mission/mission_logic.[ch] (issue #36). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
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
    in->travel_counts = 1000u;
    *t += 10u; step(m, in, *t);
    in->motion_action = m->current_action;
    in->motion_status = RD_MOTION_DONE;
    in->travel_counts += length;
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
    assert(m.state == MISSION_NAVIGATE && !out.stop_now);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    assert(motion_cmd_target(&out.command) - MOTION_DIST_TOL_MM >= 25.0f);
    assert(motion_cmd_speed(&out.command) == (unsigned)MISSION_BROAD_SPEED_MMPS);
    uint8_t id = motion_cmd_action(&out.command);

    in.now_ms = t + 10u;
    mission_step(&m, &in, &out);
    assert(out.command.sampled_ms == t && motion_cmd_action(&out.command) == id);
    /* Early visible sensing does not shorten the distance action. */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.motion_action = id;
    in.motion_status = RD_MOTION_RUNNING;
    out = step(&m, &in, t + 20u);
    assert(m.state == MISSION_NAVIGATE);
    assert(motion_cmd_action(&out.command) == id);
    in.motion_action = (uint8_t)(id + 1u);
    in.motion_status = RD_MOTION_DONE;
    step(&m, &in, t + 30u);
    assert(m.state == MISSION_NAVIGATE);
    in.motion_action = id;
    out = step(&m, &in, t + 40u);
    assert(m.state == MISSION_FOLLOW_LINE && !m.in_broad);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    in.line = IR_LINE_BROAD_BLACK;
    in.guidance_valid = false;
    out = step(&m, &in, t + 50u);
    assert(m.state == MISSION_NAVIGATE && motion_cmd_action(&out.command) != id);

    in.stop_down = true;
    out = step(&m, &in, t + 60u);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* Failed action, unusable sample, obstacle and deadline stop crossing. */
    for (unsigned scenario = 0u; scenario < 4u; ++scenario)
    {
        in = base(100u);
        to_follow(&m, &in);
        t = UINT32_MAX - 100u;
        in.line = IR_LINE_BROAD_BLACK;
        in.guidance_valid = false;
        step(&m, &in, t);
        in.motion_action = m.current_action;
        if (scenario == 0u)
        {
            in.motion_status = RD_MOTION_FAILED;
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
            step(&m, &in, t + MOTION_ACTION_TIMEOUT_MS - 1u);
            assert(m.state == MISSION_NAVIGATE);
            t += MOTION_ACTION_TIMEOUT_MS - 10u;
        }
        out = step(&m, &in, t + 10u);
        assert(m.state == MISSION_STOPPED && out.stop_now);
    }
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
    for (unsigned i = 1u; i < MISSION_CONFIRM_SAMPLES; ++i)
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
    assert(ms.state == MISSION_NAVIGATE && ms.phase == JUNCTION_PHASE_CONFIRM);
    for (unsigned i = 0u; i < MISSION_CONFIRM_SAMPLES; ++i)
    {
        step(&ms, &is, t += 20u);
    }
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
    in.travel_counts = 0xFFF0u;
    step(&m, &in, t += 10u);
    in.travel_counts = (0xFFF0u + 120u) & ENC_COUNT_MASK;
    MissionOutputs out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_NAVIGATE && !out.stop_scanner);
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
    for (unsigned i = 0u; i < MISSION_CONFIRM_SAMPLES; ++i)
    {
        step(&m, &in, t += 20u);
    }
    assert(m.state == MISSION_FOLLOW_LINE);
}

static MissionOutputs enter_line_recovery(Mission *m, MissionInputs *in,
    uint32_t *t)
{
    in->line = IR_LINE_LOST;
    in->guidance_valid = false;
    MissionOutputs out = step(m, in, *t += 10u);
    assert(m->state == MISSION_FOLLOW_LINE);
    for (unsigned elapsed = 0u; elapsed < MISSION_LINE_LOST_MS; elapsed += 10u)
    {
        out = step(m, in, *t += 10u);
        assert(m->state == MISSION_FOLLOW_LINE);
        assert(!out.stop_now);
    }
    return step(m, in, *t += 1u); /* strictly more than one second */
}

static void test_line_loss_qualification(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line_error = 80;
    MissionOutputs out = step(&m, &in, t += 10u);
    int steer = motion_cmd_steering(&out.command);
    unsigned speed = motion_cmd_speed(&out.command);
    assert(steer != 0);
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    for (unsigned i = 0u; i < 80u; ++i)
    {
        out = step(&m, &in, t += 10u);
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(motion_cmd_steering(&out.command) == steer);
        assert(motion_cmd_speed(&out.command) == speed);
        assert(out.command.sampled_ms == in.line_sampled_ms);
    }
    /* A single visible sample returns control to PID and resets the timer. */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_FOLLOW_LINE && !m.line_loss_pending);
    out = enter_line_recovery(&m, &in, &t);
    assert(m.state == MISSION_REACQUIRE);

    /* Zero acquisition time and a timer crossing UINT32_MAX both work. */
    const uint32_t starts[] = {UINT32_MAX - 9u, UINT32_MAX - 500u};
    for (unsigned i = 0u; i < sizeof(starts) / sizeof(starts[0]); ++i)
    {
        in = base(starts[i]);
        to_follow(&m, &in);
        t = starts[i];
        out = enter_line_recovery(&m, &in, &t);
        assert(m.state == MISSION_REACQUIRE);
    }

    /* Missing acquisition intervals cannot be counted as continuous loss. */
    in = base(100u);
    t = to_follow(&m, &in);
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    step(&m, &in, t += 10u);
    out = step(&m, &in, t += MISSION_LINE_LOST_MS + 1u);
    assert(m.state == MISSION_FOLLOW_LINE);
    assert(m.line_lost_since_ms == in.line_sampled_ms);
    /* Reusing that cached sample cannot start recovery; it becomes stale. */
    in.now_ms += IR_MAX_GAP_MS + 1u;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(!m.line_loss_pending);

    in = base(100u);
    t = to_follow(&m, &in);
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    step(&m, &in, t += 10u);
    in.stop_down = true;
    out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_STOPPED && out.stop_now);
    assert(!m.line_loss_pending);
}

static void test_line_loss_recovery_is_bounded(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    MissionOutputs out = enter_line_recovery(&m, &in, &t);
    assert(m.state == MISSION_REACQUIRE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    assert(motion_cmd_steering(&out.command) == -RECOVERY_STEER_Q);
    float inner_mmps = motion_cmd_speed(&out.command) *
        (1.0f - (float)RECOVERY_STEER_Q / 127.0f);
    float tracking_curve_inner = MISSION_FOLLOW_SPEED_MMPS *
        (1.0f - LINE_CURVE_SLOWDOWN) * (1.0f - MAX_STEER);
    assert(inner_mmps >= tracking_curve_inner);

    /* A repeated acquisition cannot complete alignment. */
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 0;
    out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_REACQUIRE);
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_REACQUIRE);
    for (unsigned i = 1u; i < MISSION_CONFIRM_SAMPLES; ++i)
    {
        out = step(&m, &in, t += 10u);
    }
    assert(m.state == MISSION_FOLLOW_LINE);
    out = step(&m, &in, t += 20u);
    assert(motion_cmd_steering(&out.command) == ref_p(0));

    enter_line_recovery(&m, &in, &t);
    out = step(&m, &in, t += RECOVERY_MS);
    assert(m.state == MISSION_STOPPED && out.stop_now && out.line_lost);
}

static void test_recovery_sweep_targets(void)
{
    recovery_control_t control;
    recovery_input_t in = {.now_ms = 100u, .sampled_ms = 100u,
        .count = {0xFFF0u, 0xFFF0u}, .functional = true};
    recovery_output_t out;
    recovery_control_begin(&control, &in);
    recovery_control_step(&control, &in, &out);
    assert(out.phase == RECOVERY_SEARCH && out.steering_q < 0);
    /* Distinct endpoint samples can overshoot the nominal target slightly.
       All wheel travel is forward and wraps through the encoder boundary. */
    const int heading_deg[] = {71, -71, 71, -71};
    const int mean_mm[] = {200, 450, 700, 950};
    for (unsigned i = 0u; i < 4u; ++i)
    {
        float difference = heading_deg[i] / 57.2957795f *
            TRACK_WIDTH_MM * ENC_COUNTS_PER_MM;
        int32_t left = (int32_t)lroundf(mean_mm[i] * ENC_COUNTS_PER_MM -
            difference * 0.5f);
        int32_t right = (int32_t)lroundf(mean_mm[i] * ENC_COUNTS_PER_MM +
            difference * 0.5f);
        in.count[0] = enc_advance(0xFFF0u, left);
        in.count[1] = enc_advance(0xFFF0u, right);
        in.now_ms += 100u;
        in.sampled_ms = in.now_ms;
        recovery_control_step(&control, &in, &out);
        assert(control.leg == i + 1u);
        if (i < 3u)
        {
            assert(out.phase == RECOVERY_SEARCH);
            assert((i & 1u) == 0u ? out.steering_q > 0 : out.steering_q < 0);
        }
        else
        {
            assert(out.phase == RECOVERY_FAILED); /* four legs exhausted */
        }
    }
    /* The overshoot allowance remains bounded. */
    in.count[0] = in.count[1] = 0u;
    recovery_control_begin(&control, &in);
    float difference = 76.0f / 57.2957795f *
        TRACK_WIDTH_MM * ENC_COUNTS_PER_MM;
    in.count[0] = (uint32_t)lroundf(200.0f * ENC_COUNTS_PER_MM - difference * 0.5f);
    in.count[1] = (uint32_t)lroundf(200.0f * ENC_COUNTS_PER_MM + difference * 0.5f);
    recovery_control_step(&control, &in, &out);
    assert(out.phase == RECOVERY_FAILED);
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
    in->travel_counts = enc_mean(in->wheel_count[0], in->wheel_count[1]);
    in->motion_action = motion_cmd_action(&out->command);
    in->motion_status = RD_MOTION_DONE;
}

static void fresh_front(MissionInputs *in, uint32_t now)
{
    in->front_valid = true;
    in->front_mm = 1000u;
    in->front_sampled_ms = now;
    in->front_sequence++;
    in->front_travel_counts = enc_mean(in->wheel_count[0], in->wheel_count[1]);
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
    in.travel_counts = 1000u;
    MissionOutputs out = step(&m, &in, t += 10u);
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_RUNNING;
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.travel_counts = 1001u;
    step(&m, &in, t += 20u);
    in.travel_counts = 1200u; /* Forced crossing travel is now on white. */
    in.motion_status = RD_MOTION_DONE;
    out = step(&m, &in, t += 20u);
    assert(m.state == MISSION_FOLLOW_LINE && m.held == BARCODE_LEFT);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
}

int main(void)
{
    test_stop_cancels_every_phase();
    test_thin_patch_does_not_consume_barcode();
    test_run_network_loss_and_hump_speed();
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_line_pid();
    test_line_history_storage();
    test_missing_echo_policy();
    test_both_black_forward();
    test_barcode_hold_and_junction();
    test_crossing_never_completes_mission();
    test_line_loss_qualification();
    test_line_loss_recovery_is_bounded();
    test_recovery_sweep_targets();
    test_bypass_full_sequence();
    test_bypass_failures();
    puts("mission: PASS");
    return 0;
}
