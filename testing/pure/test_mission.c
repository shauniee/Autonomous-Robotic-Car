/* Host test for app_program/subsystem3/mission_logic.[ch] (issue #36). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/subsystem3/mission_logic.h"

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

/* Drive a Mission to FOLLOW through the deliberate start press. */
static uint32_t to_follow(Mission *m, MissionInputs *in)
{
    uint32_t t = 100u;
    mission_init(m, t);
    MissionOutputs out = step(m, in, t);			/* calibrated */
    assert(m->state == MISSION_READY && out.command.mode == MOTION_MODE_STOP);
    t += 10u; in->start_pressed = true;
    out = step(m, in, t);					/* start press */
    assert(out.request_reset && m->state == MISSION_READY && out.command.mode == MOTION_MODE_STOP);
    in->start_pressed = false;
    t += 10u; in->motion_stopped = false;			/* Motion released the latch */
    out = step(m, in, t);
    assert(m->state == MISSION_FOLLOW && out.start_scanner);
    return t;
}

static void test_start_and_reset_never_move(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* A press while stopped only releases the latch; no motion command results. */
    in.stop_down = true; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_STOP && out.stop_now && out.stop_scanner && out.command.mode == MOTION_MODE_STOP);
    in.stop_down = false; in.motion_stopped = true; in.start_pressed = true; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_STOP && out.request_reset && out.command.mode == MOTION_MODE_STOP);
    in.start_pressed = false; in.motion_stopped = false; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && out.command.mode == MOTION_MODE_STOP);
    t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && out.command.mode == MOTION_MODE_STOP);	/* still waiting for a start press */

    /* A start press that Motion never honours times out instead of following. */
    in.motion_stopped = true; in.start_pressed = true; t += 10u;
    step(&m, &in, t);
    in.start_pressed = false; t += MISSION_START_WAIT_MS;
    out = step(&m, &in, t);
    assert(m.state == MISSION_READY && out.command.mode == MOTION_MODE_STOP);
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
    assert(out.start_refused && !out.request_reset && !m.start_pending && out.command.mode == MOTION_MODE_STOP);

    /* A start-blocking fault refuses the press the same way. */
    in.mapped = true; in.start_blocked = true; t += 10u;
    out = step(&m, &in, t);
    assert(out.start_refused && !m.start_pending);

    /* A fault that appears while waiting for Motion cancels the pending start. */
    in.start_blocked = false; t += 10u;
    out = step(&m, &in, t);
    assert(!out.start_refused && m.start_pending);
    in.start_pressed = false; in.start_blocked = true; t += 10u;
    out = step(&m, &in, t);
    assert(out.start_refused && !m.start_pending && m.state == MISSION_READY);
}

static void test_follow_and_stops(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.steering = IR_SCALE_ONE; t += 10u;			/* full right saturates at MAX_STEER */
    MissionOutputs out = step(&m, &in, t);
    assert(out.command.mode == MOTION_MODE_FOLLOW && out.command.steering == MAX_STEER);
    assert(out.command.sampled_ms == t && out.command.speed_mmps == MISSION_FOLLOW_SPEED_MMPS);
    in.steering = -13; t += 10u;
    out = step(&m, &in, t);
    assert(out.command.steering == -13.0f / IR_SCALE_ONE);	/* converted once, at the S2 boundary */

    /* Unusable inputs or a Safety latch stop; Safety's own stop is not repeated. */
    in.inputs_ok = false; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_STOP && out.stop_now);
    Mission m2; MissionInputs in2 = base(100u);
    t = to_follow(&m2, &in2);
    in2.safety_latched = true; t += 10u;
    out = step(&m2, &in2, t);
    assert(m2.state == MISSION_STOP && !out.stop_now && out.command.mode == MOTION_MODE_STOP);
}

static void cross_junction(Mission *m, MissionInputs *in, uint32_t *t, uint32_t length)
{
    in->line = IR_LINE_BROAD_BLACK;
    in->travel_counts = 1000u;
    *t += 10u; step(m, in, *t);
    in->travel_counts += length;
    *t += 10u; step(m, in, *t);
    in->line = IR_LINE_VISIBLE;
}

static void test_barcode_hold_and_junction(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* A barcode is held, not executed, until the next cross junction. */
    in.have_barcode = true; in.barcode_action = BARCODE_LEFT; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_FOLLOW && out.command.mode == MOTION_MODE_FOLLOW && m.held == BARCODE_LEFT);
    in.have_barcode = false;

    cross_junction(&m, &in, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_JUNCTION_TURN && out.command.mode == MOTION_MODE_TURN && out.command.target == 90.0f);
    uint8_t turn_id = out.command.action_id;
    assert(turn_id >= 2u && m.held == BARCODE_UNMAPPED);

    /* The next action starts only after DONE for the turn's ID. */
    in.motion_status = RD_MOTION_RUNNING; in.motion_action = turn_id; t += 10u;
    out = step(&m, &in, t);
    assert(out.command.mode == MOTION_MODE_TURN && out.command.action_id == turn_id);
    in.motion_status = RD_MOTION_DONE; t += 10u;
    out = step(&m, &in, t);
    assert(out.command.mode == MOTION_MODE_DISTANCE && out.command.action_id != turn_id);
    uint8_t clear_id = out.command.action_id;
    in.motion_action = clear_id; in.motion_status = RD_MOTION_DONE; t += 10u;
    out = step(&m, &in, t);
    assert(m.state == MISSION_FOLLOW);

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
        assert(mm.state == MISSION_JUNCTION_TURN && out.command.target == turns[i].deg);
    }
    Mission ms; MissionInputs is = base(100u);
    t = to_follow(&ms, &is);
    cross_junction(&ms, &is, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u; step(&ms, &is, t);
    assert(ms.state == MISSION_FOLLOW);
    /* A short broad-black blip is not a junction: the held command stays held. */
    Mission mb; MissionInputs ib = base(100u);
    t = to_follow(&mb, &ib);
    ib.have_barcode = true; ib.barcode_action = BARCODE_RIGHT; t += 10u; step(&mb, &ib, t); ib.have_barcode = false;
    cross_junction(&mb, &ib, &t, MISSION_JUNCTION_MIN_COUNTS - 1u);
    t += 10u; step(&mb, &ib, t);
    assert(mb.state == MISSION_FOLLOW && mb.held == BARCODE_RIGHT);

    /* A failed turn stops. */
    Mission mf; MissionInputs iff = base(100u);
    t = to_follow(&mf, &iff);
    iff.have_barcode = true; iff.barcode_action = BARCODE_LEFT; t += 10u; step(&mf, &iff, t); iff.have_barcode = false;
    cross_junction(&mf, &iff, &t, MISSION_JUNCTION_MIN_COUNTS);
    t += 10u; out = step(&mf, &iff, t);
    iff.motion_action = out.command.action_id; iff.motion_status = RD_MOTION_FAILED; t += 10u;
    out = step(&mf, &iff, t);
    assert(mf.state == MISSION_STOP && out.stop_now);
}

static void test_finish(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.line = IR_LINE_BROAD_BLACK; in.travel_counts = 0xFFF0u; t += 10u; step(&m, &in, t);
    in.travel_counts = (0xFFF0u + MISSION_FINISH_COUNTS) & ENC_COUNT_MASK;	/* across the wrap */
    t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_COMPLETE && out.stop_scanner && out.command.mode == MOTION_MODE_STOP);
}

static void test_line_loss_recovery_is_bounded(void)
{
    Mission m; MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);

    /* No known side: stop immediately. */
    in.line = IR_LINE_LOST; t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_STOP && out.stop_now && out.line_lost);

    /* Known side: pivot toward it, then rejoin after two fresh visible reads. */
    Mission m2; MissionInputs in2 = base(100u);
    t = to_follow(&m2, &in2);
    in2.steering = 25; t += 10u; step(&m2, &in2, t);
    in2.line = IR_LINE_LOST; t += 10u;
    out = step(&m2, &in2, t);
    assert(m2.state == MISSION_FOLLOW && out.command.mode == MOTION_MODE_FOLLOW && out.command.steering == 1.0f);
    in2.line = IR_LINE_VISIBLE; t += 10u; step(&m2, &in2, t);
    assert(m2.recovering);
    t += 10u; out = step(&m2, &in2, t);
    assert(!m2.recovering && out.command.steering == 25.0f / IR_SCALE_ONE);

    /* Never finding it again hits the bound and stops. */
    Mission m3; MissionInputs in3 = base(100u);
    t = to_follow(&m3, &in3);
    in3.steering = -25; t += 10u; step(&m3, &in3, t);
    in3.line = IR_LINE_LOST;
    for (uint32_t waited = 0u; waited < RECOVERY_MS + 20u && m3.state == MISSION_FOLLOW; waited += 10u) {
        t += 10u; out = step(&m3, &in3, t);
    }
    assert(m3.state == MISSION_STOP && out.stop_now && out.line_lost);
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
    in->motion_action = prev->command.action_id;
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
    assert(m->state == MISSION_BYPASS && out.command.mode == MOTION_MODE_STOP);
    out = bstep(m, in, t);
    assert(!out.scan_request && out.command.mode == MOTION_MODE_STOP);	/* not stationary yet */
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
    assert(m.bypass_side == 1 && out.command.mode == MOTION_MODE_TURN && out.command.target == 90.0f);
    in.front_valid = false;
    out = finish(&m, &in, &t, &out);				/* turn out done -> offset leg */
    assert(out.command.mode == MOTION_MODE_DISTANCE && out.command.target == MISSION_BYPASS_OFFSET_MM);
    out = finish(&m, &in, &t, &out);				/* offset done -> turn parallel */
    assert(out.command.mode == MOTION_MODE_TURN && out.command.target == -90.0f);
    in.travel_counts = 5000u;
    out = finish(&m, &in, &t, &out);				/* parallel leg, side-watch toward the obstacle */
    assert(m.bypass_step == BYPASS_PASS && out.command.mode == MOTION_MODE_DISTANCE);
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
    assert(out.command.mode == MOTION_MODE_TURN && out.command.target == -90.0f);
    out = finish(&m, &in, &t, &out);				/* turn in done -> offset back */
    assert(out.command.mode == MOTION_MODE_DISTANCE);
    out = finish(&m, &in, &t, &out);				/* offset done -> face forward */
    assert(out.command.mode == MOTION_MODE_TURN && out.command.target == 90.0f);
    out = finish(&m, &in, &t, &out);
    assert(m.bypass_step == BYPASS_REACQUIRE);
    in.line = IR_LINE_LOST;					/* creeping forward, no line yet */
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_BYPASS && out.command.mode == MOTION_MODE_FOLLOW);
    in.line = IR_LINE_VISIBLE;
    bstep(&m, &in, &t);
    assert(m.state == MISSION_BYPASS);				/* one read is not enough */
    bstep(&m, &in, &t);
    assert(m.state == MISSION_FOLLOW);				/* two fresh reads reacquire the original line */
}

static void test_bypass_failures(void)
{
    Mission m; MissionInputs in = base(100u); uint32_t t;
    /* Neither side wide enough. */
    into_scanning(&m, &in, &t, MISSION_BYPASS_MIN_CLEAR_MM - 1u, MISSION_BYPASS_MIN_CLEAR_MM - 1u);
    MissionOutputs out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOP && out.stop_now);

    /* Right side chosen when it has more room; scan failure stops. */
    Mission r; MissionInputs ir = base(100u);
    into_scanning(&r, &ir, &t, 400u, 900u);
    out = bstep(&r, &ir, &t);
    assert(r.bypass_side == -1 && out.command.target == -90.0f);
    Mission f; MissionInputs iff = base(100u);
    into_scanning(&f, &iff, &t, 800u, 800u);
    iff.scan_state = S5_SCAN_FAILED;
    bstep(&f, &iff, &t);
    assert(f.state == MISSION_STOP);

    /* A second obstacle while driving the offset leg stops the robot. */
    Mission s; MissionInputs is = base(100u);
    into_scanning(&s, &is, &t, 800u, 400u);
    out = bstep(&s, &is, &t); is.front_valid = false;
    out = finish(&s, &is, &t, &out);				/* offset leg running */
    assert(s.bypass_step == BYPASS_OFFSET_OUT);
    is.front_valid = true; is.front_age_ms = 0u; is.front_mm = MISSION_OBSTACLE_STOP_MM;
    out = bstep(&s, &is, &t);
    assert(s.state == MISSION_STOP && out.stop_now);
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
    ih.motion_action = out.command.action_id; ih.motion_status = RD_MOTION_FAILED;
    out = bstep(&h, &ih, &t);
    assert(h.state == MISSION_STOP && out.stop_now);

    /* The whole bypass is bounded in time. */
    Mission k; MissionInputs ik = base(100u);
    into_scanning(&k, &ik, &t, 800u, 400u);
    t += MISSION_BYPASS_TIMEOUT_MS;
    out = step(&k, &ik, t);
    assert(k.state == MISSION_STOP && out.stop_now);
}

int main(void)
{
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_barcode_hold_and_junction();
    test_finish();
    test_line_loss_recovery_is_bounded();
    test_bypass_full_sequence();
    test_bypass_failures();
    puts("mission: PASS");
    return 0;
}
