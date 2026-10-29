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
    t += 20u; step(&m, &in, t);
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
    assert(m.state == MISSION_FOLLOW_LINE && !m.in_broad);
}

static void test_line_loss_recovery_is_bounded(void)
{
    const int search_q = (int)lroundf(MAX_STEER * IR_SCALE_ONE);
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* Healthy white is line loss, never invented proof of centering. Without
       an observed side, stop until the configured sweep policy is resolved. */
    in.line = IR_LINE_LOST; in.guidance_valid = false; in.line_error = 0;
    MissionOutputs out = step(&m, &in, t += 10u);
    assert(m.state == MISSION_STOPPED && out.stop_now && out.line_lost);

    /* Known side: after the gap, search toward it on an arc with both wheels
       turning, then rejoin after two fresh visible reads. */
    Mission m2; MissionInputs in2 = base(100u);
    t = to_follow(&m2, &in2);
    in2.line_error = 25; t += 10u; step(&m2, &in2, t);
    in2.line = IR_LINE_LOST; in2.guidance_valid = false; in2.line_error = 0;
    while ((m2.state != MISSION_REACQUIRE) && m2.state == MISSION_FOLLOW_LINE) {
        t += 10u; out = step(&m2, &in2, t);
    }
    assert(m2.state == MISSION_REACQUIRE && motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW &&
           motion_cmd_steering(&out.command) == search_q && search_q < IR_SCALE_ONE);	/* inner wheel still turns */
    in2.line = IR_LINE_VISIBLE; in2.guidance_valid = true; in2.line_error = 25; t += 10u; step(&m2, &in2, t);
    assert((m2.state == MISSION_REACQUIRE));
    t += 10u; out = step(&m2, &in2, t);
    assert((m2.state != MISSION_REACQUIRE) && m2.state == MISSION_FOLLOW_LINE);
    out = step(&m2, &in2, t += 20u);
    assert(motion_cmd_steering(&out.command) == ref_p(25));	/* PID restarts after the search */

    /* Never finding it again hits the bound and stops. */
    Mission m3; MissionInputs in3 = base(100u);
    t = to_follow(&m3, &in3);
    in3.line_error = -25; t += 10u; step(&m3, &in3, t);
    in3.line = IR_LINE_LOST; in3.guidance_valid = false; in3.line_error = 0;
    for (uint32_t waited = 0u; waited < RECOVERY_MS + 20u && m3.state != MISSION_STOPPED; waited += 10u) {
        t += 10u; out = step(&m3, &in3, t);
    }
    assert(m3.state == MISSION_STOPPED && out.stop_now && out.line_lost);
}

/* ---- Bypass ------------------------------------------------------------ */

static MissionOutputs bstep(Mission *m, MissionInputs *in, uint32_t *t)
{
    *t += 10u;
    return step(m, in, *t);
}

/* Complete the current Motion action: report DONE for its ID, then step. */
static MissionOutputs finish(Mission *m, MissionInputs *in, uint32_t *t, const MissionOutputs *prev)
{
    in->motion_action = motion_cmd_action(&prev->command);
    in->motion_status = RD_MOTION_DONE;
    MissionOutputs out = bstep(m, in, t);
    in->motion_status = RD_MOTION_RUNNING;
    return out;
}

static void into_scanning(Mission *m, MissionInputs *in, uint32_t *t, uint32_t left_mm, uint32_t right_mm)
{
    *t = to_follow(m, in);
    in->front_valid = true; in->front_age_ms = 0u; in->front_mm = MISSION_OBSTACLE_STOP_MM;
    MissionOutputs out = bstep(m, in, t);
    assert(m->state == MISSION_SCAN && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = bstep(m, in, t);
    assert(!out.scan_request && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);	/* not stationary yet */
    in->stationary = true;
    out = bstep(m, in, t);
    assert(out.scan_request && m->bypass_step == BYPASS_SCANNING);
    in->scan_state = S5_SCAN_DONE; in->profile_valid = true;
    in->left_clearance_mm = left_mm; in->right_clearance_mm = right_mm;
}

static void test_bypass_full_sequence(void)
{
    Mission m; MissionInputs in = base(100u); uint32_t t;
    into_scanning(&m, &in, &t, 800u, 400u);			/* left side has more room */
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.bypass_side == 1 && motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == 90.0f);
    in.front_valid = false;
    out = finish(&m, &in, &t, &out);				/* turn out done -> offset leg */
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE && motion_cmd_target(&out.command) == MISSION_BYPASS_OFFSET_MM);
    out = finish(&m, &in, &t, &out);				/* offset done -> turn parallel */
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == -90.0f);
    in.travel_counts = 5000u;
    out = finish(&m, &in, &t, &out);				/* parallel leg, side-watch toward the obstacle */
    assert(m.bypass_step == BYPASS_PASS && motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    out = bstep(&m, &in, &t);
    assert(out.side_watch_on && out.side_watch_deg == -SERVO_MAX_DEG);	/* obstacle is on the right */
    in.side_valid = true; in.side_mm = 300u;
    out = bstep(&m, &in, &t);					/* baseline captured */
    assert(m.bypass_step == BYPASS_PASS);
    in.travel_counts += 150u; in.side_mm = 300u + MISSION_PASS_JUMP_MM;	/* at the threshold: not yet */
    out = bstep(&m, &in, &t);
    assert(m.bypass_step == BYPASS_PASS);
    in.travel_counts += 20u; in.side_mm = 300u + MISSION_PASS_JUMP_MM + 1u;	/* jump: far end passed */
    out = bstep(&m, &in, &t);
    assert(m.bypass_step == BYPASS_PASS_MARGIN && out.event_kind == MQTT_EVENT_OBSTACLE &&
           out.event_arg2 == (uint16_t)(170.0f / ENC_COUNTS_PER_MM + 0.5f));	/* depth in mm from encoder travel */
    out = bstep(&m, &in, &t);
    assert(!out.side_watch_on);
    out = finish(&m, &in, &t, &out);				/* margin done -> turn in */
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == -90.0f);
    out = finish(&m, &in, &t, &out);				/* turn in done -> offset back */
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    out = finish(&m, &in, &t, &out);				/* offset done -> face forward */
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN && motion_cmd_target(&out.command) == 90.0f);
    out = finish(&m, &in, &t, &out);
    assert(m.bypass_step == BYPASS_REACQUIRE);
    in.line = IR_LINE_LOST;					/* creeping forward, no line yet */
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_BYPASS && motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    in.line = IR_LINE_VISIBLE;
    bstep(&m, &in, &t);
    assert(m.state == MISSION_BYPASS);				/* one read is not enough */
    bstep(&m, &in, &t);
    assert(m.state == MISSION_FOLLOW_LINE);				/* two fresh reads reacquire the original line */
}

static void test_bypass_failures(void)
{
    Mission m; MissionInputs in = base(100u); uint32_t t;
    /* Neither side wide enough. */
    into_scanning(&m, &in, &t, MISSION_BYPASS_MIN_CLEAR_MM - 1u, MISSION_BYPASS_MIN_CLEAR_MM - 1u);
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* Right side chosen when it has more room; scan failure stops. */
    Mission r; MissionInputs ir = base(100u);
    into_scanning(&r, &ir, &t, 400u, 900u);
    out = bstep(&r, &ir, &t);
    assert(r.bypass_side == -1 && motion_cmd_target(&out.command) == -90.0f);
    Mission f; MissionInputs iff = base(100u);
    into_scanning(&f, &iff, &t, 800u, 800u);
    iff.scan_state = S5_SCAN_FAILED;
    bstep(&f, &iff, &t);
    assert(f.state == MISSION_STOPPED);

    /* A second obstacle while driving the offset leg stops the robot. */
    Mission s; MissionInputs is = base(100u);
    into_scanning(&s, &is, &t, 800u, 400u);
    out = bstep(&s, &is, &t); is.front_valid = false;
    out = finish(&s, &is, &t, &out);				/* offset leg running */
    assert(s.bypass_step == BYPASS_OFFSET_OUT);
    is.front_valid = true; is.front_age_ms = 0u; is.front_mm = MISSION_OBSTACLE_STOP_MM;
    out = bstep(&s, &is, &t);
    assert(s.state == MISSION_STOPPED && out.stop_now);
    /* Stale front reading is not trusted either way: no stop on old data. */
    Mission g; MissionInputs ig = base(100u);
    into_scanning(&g, &ig, &t, 800u, 400u);
    out = bstep(&g, &ig, &t); ig.front_valid = false;
    out = finish(&g, &ig, &t, &out);
    ig.front_valid = true; ig.front_age_ms = 10u * SONAR_REPEAT_MS; ig.front_mm = 10u;
    bstep(&g, &ig, &t);
    assert(g.state == MISSION_BYPASS);

    /* A failed leg or a pass leg that never passes the obstacle stops. */
    Mission h; MissionInputs ih = base(100u);
    into_scanning(&h, &ih, &t, 800u, 400u);
    out = bstep(&h, &ih, &t); ih.front_valid = false;
    ih.motion_action = motion_cmd_action(&out.command); ih.motion_status = RD_MOTION_FAILED;
    out = bstep(&h, &ih, &t);
    assert(h.state == MISSION_STOPPED && out.stop_now);

    /* The whole bypass is bounded in time. */
    Mission k; MissionInputs ik = base(100u);
    into_scanning(&k, &ik, &t, 800u, 400u);
    t += MISSION_BYPASS_TIMEOUT_MS;
    out = step(&k, &ik, t);
    assert(k.state == MISSION_STOPPED && out.stop_now);
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
    assert(m2.state == MISSION_STOPPED && !m2.line_pid_primed && m2.line_integral == 0.0f);
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

int main(void)
{
    test_stop_cancels_every_phase();
    test_run_network_loss_and_hump_speed();
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_line_pid();
    test_both_black_forward();
    test_barcode_hold_and_junction();
    test_crossing_never_completes_mission();
    test_line_loss_recovery_is_bounded();
    test_bypass_full_sequence();
    test_bypass_failures();
    puts("mission: PASS");
    return 0;
}
