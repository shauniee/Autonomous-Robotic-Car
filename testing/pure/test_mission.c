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
    int correction = LINE_EDGE_Q; /* first touch, no bend */
    return error_q < 0 ? -correction : (error_q > 0 ? correction : 0);
}

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
    /* 100 mm/s guardrail: steering never lowers the commanded course speed. */
    assert(out.command.sampled_ms == t && motion_cmd_speed(&out.command) == 100u);

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

static void advance_mm(MissionInputs *in, float mm)
{
    int counts = (int)ceilf(mm * ENC_COUNTS_PER_MM); /* never short of mm */
    in->wheel_count[0] = enc_advance(in->wheel_count[0], counts);
    in->wheel_count[1] = enc_advance(in->wheel_count[1], counts);
}

/* Raw-consistent line states: error and black bits agree. */
static void see(MissionInputs *in, int side)
{
    in->line = IR_LINE_VISIBLE;
    in->guidance_valid = true;
    in->line_error = (int8_t)(side * IR_SCALE_ONE);
    in->left_black = side < 0;
    in->right_black = side > 0;
}

static MissionOutputs enter_line_recovery(Mission *m, MissionInputs *in,
    uint32_t *t)
{
    see(in, -1);
    MissionOutputs out = step(m, in, *t += IR_SAMPLE_MS);
    assert(m->state == MISSION_FOLLOW_LINE && m->line_side < 0);
    see(in, 0);
    advance_mm(in, LINE_LOST_MM - 1);
    out = step(m, in, *t += IR_SAMPLE_MS);
    assert(m->state == MISSION_FOLLOW_LINE);
    advance_mm(in, 1);
    out = step(m, in, *t += IR_SAMPLE_MS);
    assert(m->state == MISSION_REACQUIRE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    return out;
}

static void test_line_loss_qualification(void)
{
    /* White/white past LINE_LOST_MM after a touch is loss; the queue is kept
       until then and cleared on entering recovery. */
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    see(&in, 1);
    step(&m, &in, t += IR_SAMPLE_MS);
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    see(&in, 0);
    MissionOutputs out;
    for (unsigned mm = 0u; mm < LINE_LOST_MM; mm += 10u)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(motion_cmd_steering(&out.command) == LINE_DRIFT_Q);
        assert(out.command.sampled_ms == in.line_sampled_ms);
        assert(!out.stop_scanner && !out.scan_cancel);
        assert(m.held == BARCODE_LEFT && m.manual_held);
        advance_mm(&in, 10.0f);
    }
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.state == MISSION_REACQUIRE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    assert(out.stop_scanner && out.scan_cancel && !out.side_watch_on);
    assert(m.held == BARCODE_UNMAPPED && !m.manual_held);

    /* Before the first touch the line is between the sensors: a long centred
       straight is never loss, however far or long. */
    in = base(100u);
    t = to_follow(&m, &in);
    for (unsigned i = 0u; i < 100u; ++i)
    {
        advance_mm(&in, 10.0f);
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE && motion_cmd_steering(&out.command) == 0);
    }

    /* A touch or a crossbar restarts the distance, even near the limit. */
    for (unsigned scenario = 0u; scenario < 2u; ++scenario)
    {
        in = base(100u);
        t = to_follow(&m, &in);
        see(&in, -1);
        step(&m, &in, t += IR_SAMPLE_MS);
        see(&in, 0);
        advance_mm(&in, LINE_LOST_MM - 10);
        step(&m, &in, t += IR_SAMPLE_MS);
        if (scenario == 0u)
        {
            see(&in, -1);
        }
        else
        {
            in.line = IR_LINE_BROAD_BLACK;
            in.guidance_valid = false;
            in.left_black = in.right_black = true;
        }
        step(&m, &in, t += IR_SAMPLE_MS);
        see(&in, 0);
        advance_mm(&in, LINE_LOST_MM - 10);
        step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE);
    }

    /* Encoder wrap qualifies correctly. */
    in = base(100u);
    t = to_follow(&m, &in);
    in.wheel_count[0] = in.wheel_count[1] = ENC_COUNT_MASK - 100u;
    enter_line_recovery(&m, &in, &t);

    /* A cached sample becomes stale and stops. */
    in = base(100u);
    t = to_follow(&m, &in);
    see(&in, -1);
    step(&m, &in, t += IR_SAMPLE_MS);
    see(&in, 0);
    out = step(&m, &in, t += IR_SAMPLE_MS);
    in.now_ms += IR_MAX_GAP_MS;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_FOLLOW_LINE);
    ++in.now_ms;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    for (unsigned scenario = 0u; scenario < 3u; ++scenario)
    {
        in = base(100u);
        t = to_follow(&m, &in);
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
    puts("line loss: distance after a touch, queue retention, restart, wrap and freshness PASS");
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
        see(&in, error < 0 ? -1 : 1);
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
        /* Returning, the left sensor crosses the line once before aligning. */
        bool crossing = m.bypass.route == BYPASS_ROUTE_LINE && !m.bypass.line_seen;
        in.left_black = crossing;
        in.line_error = crossing ? -IR_SCALE_ONE : 0;
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
    assert(normal == 100u);
    in.hump_active = true;
    out = step(&m, &in, t += 20u);
    assert(motion_cmd_speed(&out.command) == 50u && m.state == MISSION_FOLLOW_LINE);
    in.hump_active = false;
    out = step(&m, &in, t += 20u);
    assert(motion_cmd_speed(&out.command) == normal);
}
int main(void)
{
    test_start_and_reset_never_move();
    test_start_gate();
    test_follow_and_stops();
    test_line_loss_qualification();
    test_line_loss_recovery_is_bounded();
    test_recovery_sweep_targets();
    test_recovery_reverses_through_motion();
    test_bypass_full_sequence();
    test_bypass_long_encoder_travel();
    test_bypass_failures();
    test_missing_echo_policy();
    test_stop_cancels_every_phase();
    test_run_network_loss_and_hump_speed();
    puts("mission retained safety/recovery/bypass: PASS");
    return 0;
}
