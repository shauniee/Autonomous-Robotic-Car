/* Host test for app_program/mission/mission_logic.[ch] (issue #36). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/common/robot_time_math.h"

/* PR149 fallback: every consumed valid barcode gets one event without
   duplicating the ordinary FOLLOW barcode event. */
static void test_barcode_event_fallback_policy(void)
{
    MissionInputs in = {.have_barcode = false, .barcode_letter = 0u};
    MissionOutputs out = {.event_kind = 0u};
    assert(!mission_barcode_event_needed(&in, &out));
    out.event_kind = MQTT_EVENT_TURN;
    assert(!mission_barcode_event_needed(&in, &out));
    in.have_barcode = true;
    in.barcode_letter = BARCODE_NO_LETTER;
    assert(!mission_barcode_event_needed(&in, &out));
    out.event_kind = 0u;
    assert(!mission_barcode_event_needed(&in, &out));
    for (uint8_t letter = 0u; letter < BARCODE_ALPHABET; ++letter)
    {
        in.barcode_letter = letter;
        out.event_kind = 0u;
        assert(mission_barcode_event_needed(&in, &out));
        out.event_kind = MQTT_EVENT_TURN;
        assert(mission_barcode_event_needed(&in, &out));
        out.event_kind = MQTT_EVENT_OBSTACLE;
        assert(mission_barcode_event_needed(&in, &out));
        out.event_kind = MQTT_EVENT_BARCODE;
        assert(!mission_barcode_event_needed(&in, &out));
    }
}

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
    in.start_pressed = false; t += 200u;
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
    assert(out.command.sampled_ms == t && motion_cmd_speed(&out.command) == COURSE_SPEED_MMPS);

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

/* The box is still there when the servo comes back forward from a sweep. */
static MissionOutputs bstep(Mission *m, MissionInputs *in, uint32_t *t)
{
    *t += 10u;
    if (m->state != MISSION_SCAN || !m->scan_phase || in->scan_id != m->scan_id ||
        (in->scan_state != OBSTACLE_SCAN_DONE && in->scan_state != OBSTACLE_SCAN_FAILED))
    {
        return step(m, in, *t);
    }
    bool valid = in->front_valid;
    uint32_t range = in->front_mm;
    in->front_valid = true;
    in->front_mm = MISSION_OBSTACLE_STOP_MM;
    in->front_sequence++;
    MissionOutputs out = step(m, in, *t);
    if (out.scan_request && out.scan_keep_narrower) /* the second sweep gives the same profile */
    {
        in->scan_id = out.scan_id;
        in->front_sequence++;
        *t += 10u;
        out = step(m, in, *t);
    }
    in->front_valid = valid;
    in->front_mm = range;
    return out;
}

/* An obstacle is a planned stop: the car slows on the profile before scanning. */
static void slow_to_rest(Mission *m, MissionInputs *in, uint32_t *t, MissionOutputs *out)
{
    assert(motion_cmd_mode(&out->command) == MOTION_MODE_DISTANCE);
    assert(motion_cmd_steering(&out->command) == MOTION_TURN_RAMPED);
    assert(motion_cmd_target(&out->command) == (int)MOTION_STOP_MM(COURSE_SPEED_MMPS));
    assert(!out->scan_request);
    in->motion_action = motion_cmd_action(&out->command);
    in->motion_status = RD_MOTION_DONE;
    *out = bstep(m, in, t);
    in->motion_status = RD_MOTION_IDLE;
}

static void test_obstacle_slow_down_can_still_stop(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.front_valid = true;
    in.front_age_ms = 0u;
    in.front_mm = MISSION_OBSTACLE_STOP_MM;
    MissionOutputs out = bstep(&m, &in, &t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    in.stop_down = true; /* a safety stop never waits for the profile */
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);

    puts("obstacle slow-down: GP21 still stops at once PASS");
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
    slow_to_rest(m, in, t, &out);
    out = bstep(m, in, t);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP && !out.scan_request);
    in->stationary = true;
    out = bstep(m, in, t);
    assert(out.scan_request && m->scan_phase == 1u);
    in->scan_id = out.scan_id;
    in->scan_state = OBSTACLE_SCAN_DONE;
    in->profile_valid = true;
    in->profile_closest_mm = 300u;
    in->profile_width_mm = 200u;
    in->profile_planning_mm = OBSTACLE_EXPECTED_MAX_WIDTH_MM;
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

/* Range of the side ray from the current pose, hitting the box's side or the background. */
static uint32_t side_ray_mm(const Mission *m, float box_front, float box_rear, float half_width,
    bool *hit)
{
    float side = m->bypass.left_side ? 1.0f : -1.0f;
    float sonar_x = m->bypass.forward_q8 / 256.0f + OBSTACLE_SONAR_FORWARD_MM;
    float across = side * m->bypass.left_q8 / 256.0f - half_width;
    float along = across / tanf(SERVO_LOOK_DEG * 0.0174532925f);
    *hit = across > 0.0f && sonar_x + along >= box_front && sonar_x + along <= box_rear;
    return *hit ? (uint32_t)lroundf(across / sinf(SERVO_LOOK_DEG * 0.0174532925f)) : 2000u;
}

static void run_bypass_sequence(uint32_t left_mm, uint32_t right_mm, float depth_mm,
    float half_width, bool background_echoes)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    into_scanning(&m, &in, &t, left_mm, right_mm);
    in.profile_forward_mm = 250; /* 145 mm range plus the sonar mount */
    in.profile_closest_mm = 250u;
    in.profile_width_mm = (uint16_t)lroundf(half_width * 2.0f);
    float box_front = 250.0f, box_rear = box_front + depth_mm;
    MissionOutputs out = bstep(&m, &in, &t);
    bool left = left_mm >= right_mm;
    assert(m.state == MISSION_BYPASS && m.bypass.left_side == left);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    assert(abs(motion_cmd_target(&out.command)) == m.bypass.out_deg);
    assert(m.bypass.out_deg > 0u && m.bypass.out_deg < 90u);
    float facing = left ? BYPASS_BODY_RIGHT_MM : BYPASS_BODY_LEFT_MM;
    bool depth_seen = false;
    uint32_t routes = 0u;
    unsigned drives = 0u;
    uint8_t driving = 0u;
    float driven = 0.0f;
    for (unsigned i = 0u; i < 3000u && m.state == MISSION_BYPASS; ++i)
    {
        routes |= 1u << m.bypass.route;
        in.stationary = true;
        in.motion_status = RD_MOTION_IDLE;
        if (motion_cmd_mode(&out.command) == MOTION_MODE_TURN)
        {
            complete_action(&in, &out);
        }
        else if (motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE)
        {
            /* A leg runs as one action, moving a little per release. */
            uint8_t id = motion_cmd_action(&out.command);
            if (id != driving)
            {
                driving = id;
                driven = 0.0f;
                ++drives;
            }
            int target = motion_cmd_target(&out.command);
            float step_mm = fminf(1.0f, (float)target - driven);
            int32_t counts = (int32_t)lroundf((driven + step_mm) * ENC_COUNTS_PER_MM) -
                (int32_t)lroundf(driven * ENC_COUNTS_PER_MM);
            in.wheel_count[0] += (uint32_t)counts;
            in.wheel_count[1] += (uint32_t)counts;
            driven += step_mm;
            in.stationary = false;
            in.motion_action = id;
            in.motion_status = driven >= (float)target ? RD_MOTION_DONE : RD_MOTION_RUNNING;
        }
        fresh_front(&in, t + 10u);
        bool hit;
        in.side_mm = side_ray_mm(&m, box_front, box_rear, half_width, &hit);
        in.side_valid = hit || background_echoes;
        in.side_sampled_ms = t + 10u;
        in.side_sequence++;
        /* The IR pair is ahead of the axle, so it reaches the line first. */
        in.left_black = m.bypass.route == BYPASS_ROUTE_RETURN &&
            fabsf(m.bypass.left_q8 / 256.0f) <= NAV_SENSOR_FORWARD_MM;
        out = bstep(&m, &in, &t);
        float across = (left ? 1.0f : -1.0f) * m.bypass.left_q8 / 256.0f;
        if (m.bypass.route == BYPASS_ROUTE_PASS || m.bypass.route == BYPASS_ROUTE_REAR)
        {
            /* The measured box edge stays the safe gap off the facing side. */
            assert(across - facing >= half_width + BYPASS_SAFE_GAP_MM - MOTION_DIST_TOL_MM);
            assert(across - facing <= half_width + BYPASS_SAFE_GAP_MM + MOTION_DIST_TOL_MM);
            assert(out.side_watch_on && out.side_watch_deg == (left ? -SERVO_LOOK_DEG : SERVO_LOOK_DEG));
        }
        if (m.bypass.route == BYPASS_ROUTE_IN)
        {
            /* Its facing side swings no further back than this, clear of the rear. */
            assert(m.bypass.forward_q8 / 256.0f - facing >= box_rear + BYPASS_SAFE_GAP_MM - 2.0f);
            /* It stops as soon as that holds, unless the ray had already passed the rear. */
            assert(depth_mm <= 100.0f ||
                m.bypass.forward_q8 / 256.0f - facing <= box_rear + BYPASS_SAFE_GAP_MM + 5.0f);
        }
        if (out.event_kind == MQTT_EVENT_OBSTACLE)
        {
            /* Reads long, never short. */
            assert((float)out.event_arg2 >= depth_mm - 2.0f);
            depth_seen = true;
        }
    }
    assert(depth_seen && m.state == MISSION_NAVIGATE);
    assert(m.phase == JUNCTION_PHASE_ADVANCE && m.turn_length_mm == NAV_SENSOR_FORWARD_MM);
    assert(m.turn_code == (left ? 1u : 2u));
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    assert(drives == 3u); /* one drive per leg, never stop and go */
    assert((routes & (1u << BYPASS_ROUTE_OFFSET)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_PASS)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_REAR)) != 0u);
    assert((routes & (1u << BYPASS_ROUTE_RETURN)) != 0u);
}

static void test_bypass_long_encoder_travel(void)
{
    bypass_control_t control;
    bypass_input_t in = {.now_ms = 100u, .stationary = true, .functional = true};
    ObstacleProfile profile = {.valid = true, .closest_mm = 500u, .width_mm = 200u,
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
    run_bypass_sequence(800u, 400u, 150.0f, 100.0f, true);
    run_bypass_sequence(400u, 900u, 150.0f, 100.0f, true);
    run_bypass_sequence(800u, 400u, 100.0f, 50.0f, true);
    run_bypass_sequence(800u, 400u, 200.0f, 75.0f, true);
    run_bypass_sequence(800u, 400u, 150.0f, 75.0f, false); /* open room: no echo past the rear */
    run_bypass_sequence(800u, 400u, 40.0f, 75.0f, true); /* ray already past a shallow box */
}

static void test_bypass_real_outline(void)
{
    bypass_control_t control;
    bypass_input_t in = {.now_ms = 100u, .stationary = true, .functional = true};
    uint8_t action = 1u;
    /* Between the two pivots' reach: only a left pivot keeps the barcode arm clear. */
    ObstacleProfile profile = {.valid = true, .closest_mm = 220u, .width_mm = 200u,
        .left_clearance_mm = 400u, .right_clearance_mm = 800u};
    assert(bypass_control_begin(&control, &in, &profile, 200u, &action));
    assert(control.left_side);
    profile.left_clearance_mm = 219u;
    assert(!bypass_control_begin(&control, &in, &profile, 200u, &action));
    profile.closest_mm = 240u;
    assert(bypass_control_begin(&control, &in, &profile, 200u, &action));
    assert(!control.left_side);
    profile.closest_mm = 210u;
    profile.left_clearance_mm = 800u;
    assert(!bypass_control_begin(&control, &in, &profile, 200u, &action));
    /* The passing offset uses the side that faces the box. */
    profile.closest_mm = 500u;
    profile.obstacle_left_mm = 100;
    assert(bypass_control_begin(&control, &in, &profile, 200u, &action));
    assert(control.left_side && control.offset_mm == 285);
    /* F sin(a) - E cos(a) = d for F 500, E 200, d 85: the least pivot, rounded up. */
    assert(control.out_deg == 32u);

    /* Forward travel keeps the arm, not the sonar, clear of what is ahead. */
    in = (bypass_input_t){.now_ms = 100u, .stationary = true, .functional = true};
    profile.obstacle_left_mm = 0;
    assert(bypass_control_begin(&control, &in, &profile, 200u, &action));
    int32_t counts = (int32_t)lroundf((float)control.out_deg * 0.0174532925f *
        TRACK_WIDTH_MM * 0.5f * ENC_COUNTS_PER_MM);
    in.count[0] = (uint32_t)-counts;
    in.count[1] = (uint32_t)counts;
    in.now_ms += 20u;
    in.motion_action = control.action_id;
    in.motion_status = RD_MOTION_DONE;
    bypass_output_t out;
    bypass_control_step(&control, &in, &action, &out);
    assert(control.route == BYPASS_ROUTE_OFFSET && control.work == BYPASS_WORK_DRIVE);
    assert(lroundf(control.action_target * sinf(control.out_deg * 0.0174532925f)) >= 185);
    in.now_ms += 20u;
    in.front_valid = true;
    in.front_mm = 109u;
    in.front_ms = in.now_ms;
    bypass_control_step(&control, &in, &action, &out);
    assert(out.route == BYPASS_ROUTE_OFFSET);
    in.front_mm = 108u;
    bypass_control_step(&control, &in, &action, &out);
    assert(out.route == BYPASS_ROUTE_FAILED);
    puts("Obstacle: bypass uses the measured outline PASS");
}

/* Scan again after a reverse, from a further position. */
static MissionOutputs front_reading(Mission *m, MissionInputs *in, uint32_t *t, bool valid, uint32_t mm)
{
    in->front_valid = valid;
    in->front_mm = mm;
    in->front_sequence++;
    *t += 10u;
    return step(m, in, *t);
}

static MissionOutputs finish_scan_at(Mission *m, MissionInputs *in, uint32_t *t, uint32_t closest)
{
    in->scan_id = m->scan_id;
    in->scan_state = OBSTACLE_SCAN_DONE;
    in->profile_closest_mm = closest;
    return bstep(m, in, t);
}

static void test_scan_obstacle_removed(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    /* Removed while the sweep ran, so the scan's profile is out of date: no bypass. */
    into_scanning(&m, &in, &t, 800u, 800u);
    in.scan_state = OBSTACLE_SCAN_FINE;
    in.front_sequence++; /* the sweep's own 0 degree ray does not count */
    t += 10u;
    (void)step(&m, &in, t);
    in.scan_state = OBSTACLE_SCAN_DONE;
    t += 10u;
    MissionOutputs out = step(&m, &in, t);
    assert(m.state == MISSION_SCAN && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = front_reading(&m, &in, &t, false, 0u);
    out = front_reading(&m, &in, &t, true, 900u);
    assert(m.state == MISSION_SCAN && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = front_reading(&m, &in, &t, false, 0u);
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_now);
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_FOLLOW_LINE && motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);

    /* A failed scan with the box gone also carries on, without the retry. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 800u);
    in.scan_state = OBSTACLE_SCAN_FAILED;
    for (unsigned i = 0u; i < MISSION_SCAN_CLEAR_SAMPLES; ++i)
    {
        out = front_reading(&m, &in, &t, false, 0u);
        assert(!out.scan_request);
    }
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_now);

    /* A dropped ping is not a removal: one blocked reading restarts the count and bypasses. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 800u);
    (void)front_reading(&m, &in, &t, false, 0u);
    (void)front_reading(&m, &in, &t, false, 0u);
    out = front_reading(&m, &in, &t, true, MISSION_OBSTACLE_STOP_MM);
    assert(m.state == MISSION_SCAN && out.scan_request && m.front_clear == 0u);
    puts("Obstacle: obstacle removed during a scan resumes the line PASS");
}

static void test_bypass_reverses_when_too_close(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    /* Only the right has room, and its pivot needs 232 mm from the axle. */
    into_scanning(&m, &in, &t, 100u, 800u);
    MissionOutputs out = finish_scan_at(&m, &in, &t, 220u);
    assert(m.state == MISSION_SCAN && m.scan_backing);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    assert(motion_cmd_target(&out.command) == -32.0f);
    complete_action(&in, &out);
    in.stationary = false;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_SCAN && !m.scan_backing && m.scan_phase == 0u);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.scan_state = OBSTACLE_SCAN_SETTLING; /* the old DONE must not satisfy the rescan */
    in.stationary = true;
    out = bstep(&m, &in, &t);
    assert(out.scan_request && m.scan_phase == 1u);
    out = finish_scan_at(&m, &in, &t, 252u);
    assert(m.state == MISSION_BYPASS && !m.bypass.left_side);

    /* No gain from the reverse (raised wheels, or the box followed): it stops at the limit. */
    in = base(100u);
    into_scanning(&m, &in, &t, 100u, 800u);
    for (unsigned total = 32u; total <= BYPASS_BACKUP_MAX_MM; total += 32u)
    {
        out = finish_scan_at(&m, &in, &t, 220u);
        assert(m.scan_backing && m.backed_mm == total);
        complete_action(&in, &out);
        (void)bstep(&m, &in, &t);
        in.scan_state = OBSTACLE_SCAN_SETTLING;
        in.stationary = true;
        out = bstep(&m, &in, &t);
        assert(out.scan_request && !out.scan_keep_narrower);
    }
    out = finish_scan_at(&m, &in, &t, 220u);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* A failed reverse stops. */
    in = base(100u);
    into_scanning(&m, &in, &t, 100u, 800u);
    out = finish_scan_at(&m, &in, &t, 220u);
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_FAILED;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* No room on either side: reverse in steps and look again, then bypass once it shows. */
    in = base(100u);
    into_scanning(&m, &in, &t, 100u, 100u);
    out = finish_scan_at(&m, &in, &t, 300u);
    assert(m.scan_backing && motion_cmd_target(&out.command) == -(float)BYPASS_NO_ROOM_STEP_MM);
    complete_action(&in, &out);
    (void)bstep(&m, &in, &t);
    in.scan_state = OBSTACLE_SCAN_SETTLING;
    in.stationary = true;
    out = bstep(&m, &in, &t);
    assert(out.scan_request);
    in.left_clearance_mm = 800u;
    out = finish_scan_at(&m, &in, &t, 350u);
    assert(m.state == MISSION_BYPASS && m.bypass.left_side);

    /* Never any room: it stops once the full reverse is used. */
    in = base(100u);
    into_scanning(&m, &in, &t, 100u, 100u);
    for (unsigned total = BYPASS_NO_ROOM_STEP_MM; total <= BYPASS_BACKUP_MAX_MM; total += BYPASS_NO_ROOM_STEP_MM)
    {
        out = finish_scan_at(&m, &in, &t, 300u);
        assert(m.scan_backing && m.backed_mm == total);
        complete_action(&in, &out);
        (void)bstep(&m, &in, &t);
        in.scan_state = OBSTACLE_SCAN_SETTLING;
        in.stationary = true;
        out = bstep(&m, &in, &t);
        assert(out.scan_request);
    }
    out = finish_scan_at(&m, &in, &t, 300u);
    assert(m.state == MISSION_STOPPED && out.stop_now);

    /* After a reverse the box reads farther than the stop distance, but it has not gone. */
    in = base(100u);
    into_scanning(&m, &in, &t, 100u, 100u);
    out = finish_scan_at(&m, &in, &t, 300u);
    complete_action(&in, &out);
    (void)bstep(&m, &in, &t);
    in.scan_state = OBSTACLE_SCAN_SETTLING;
    in.stationary = true;
    out = bstep(&m, &in, &t);
    in.scan_id = m.scan_id;
    in.scan_state = OBSTACLE_SCAN_DONE;
    out = front_reading(&m, &in, &t, true, MISSION_OBSTACLE_STOP_MM + BYPASS_NO_ROOM_STEP_MM);
    assert(m.state == MISSION_SCAN && m.scan_backing && m.front_clear == 0u);

    /* Two sweeps: the second asks to keep the narrower profile, and if it fails the
       first still stands. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 800u);
    out = front_reading(&m, &in, &t, true, MISSION_OBSTACLE_STOP_MM);
    assert(m.state == MISSION_SCAN && out.scan_request && out.scan_keep_narrower);
    in.scan_id = out.scan_id;
    in.scan_state = OBSTACLE_SCAN_FAILED;
    out = front_reading(&m, &in, &t, true, MISSION_OBSTACLE_STOP_MM);
    assert(m.state == MISSION_BYPASS && !out.stop_now);
    puts("Obstacle: too close or no room reverses within the limit, two sweeps PASS");
}

static void test_bypass_failures(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t;
    MissionOutputs out;

    /* Wrong scan IDs cannot release movement; failed scans retry boundedly. */
    in = base(100u);
    into_scanning(&m, &in, &t, 800u, 400u);
    in.scan_id = (uint8_t)(m.scan_id + 1u);
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_SCAN && motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.scan_state = OBSTACLE_SCAN_FAILED;
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
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    in.motion_status = RD_MOTION_RUNNING;
    fresh_front(&in, t + 10u);
    in.front_valid = false;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_BYPASS);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE);
    puts("INT-24 no echo: PASS (report-only while tracking and on a bounded bypass leg)");
}

/* Each read gives one route event: Mission's own while following, otherwise the
   task's fallback, including when another event replaces Mission's (INT-154). */
static void test_one_barcode_event_per_read(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    MissionOutputs out = bstep(&m, &in, &t);
    assert(!mission_barcode_event_needed(&in, &out));	/* no read */

    in.have_barcode = true;
    in.barcode_letter = 3u;
    in.barcode_action = BARCODE_LEFT;
    out = bstep(&m, &in, &t);
    assert(m.state == MISSION_FOLLOW_LINE && out.event_kind == MQTT_EVENT_BARCODE);
    assert(out.event_arg == 3u && !mission_barcode_event_needed(&in, &out));

    MissionOutputs other = {.event_kind = MQTT_EVENT_TURN};
    assert(mission_barcode_event_needed(&in, &other));
    other.event_kind = 0u;			/* a read outside following */
    assert(mission_barcode_event_needed(&in, &other));
    in.barcode_letter = BARCODE_NO_LETTER;	/* no decoded letter to report */
    assert(!mission_barcode_event_needed(&in, &other));
    puts("barcode events: PASS (one per read)");
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

static void test_run_network_loss(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    in.start_blocked = true; /* connectivity lost after accepted local start */
    MissionOutputs out = step(&m, &in, t += 20u);
    assert(m.state == MISSION_FOLLOW_LINE && !out.stop_now);
    assert(motion_cmd_speed(&out.command) == COURSE_SPEED_MMPS);
}
int main(void)
{
    test_barcode_event_fallback_policy();
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
    test_bypass_real_outline();
    test_bypass_reverses_when_too_close();
    test_scan_obstacle_removed();
    test_missing_echo_policy();
    test_one_barcode_event_per_read();
    test_obstacle_slow_down_can_still_stop();
    test_stop_cancels_every_phase();
    test_run_network_loss();
    puts("mission retained safety/recovery/bypass: PASS");
    return 0;
}
