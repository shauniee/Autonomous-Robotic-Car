#include <assert.h>
#include <stdio.h>
#include "app_program/mission/mission_logic.h"

static void sample(MissionInputs *in, unsigned left, unsigned right, uint32_t now)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    ir_tracking_sample(&tracker, (IrRaw){left, right}, true, now);
    S3IrSnapshot s = ir_snapshot_build(&tracker, 1u, now);
    in->now_ms = now;
    in->line_sampled_ms = s.sampled_ms;
    in->line = s.line;
    in->line_error = s.error;
    in->usable_line = s.normalized_valid;
    in->guidance_valid = s.guidance_valid;
}

int main(void)
{
    MissionInputs in = {.inputs_ok = true, .mapped = true};
    Mission m;
    MissionOutputs out;
    mission_init(&m, 100u);
    sample(&in, 0u, 0u, 100u);
    in.start_pressed = true;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_FOLLOW_LINE);
    in.start_pressed = false;
    const unsigned raw[][2] = {{1u, 0u}, {0u, 1u}, {0u, 0u}};
    for (unsigned i = 0; i < 3u; ++i)
    {
        sample(&in, raw[i][0], raw[i][1], 120u + 20u * i);
        mission_step(&m, &in, &out);
        MotionController control;
        motion_control_reset(&control);
        MotionInput motion_in = {.now_ms = in.now_ms};
        MotionOutput wheel = {0};
        motion_control_step(&control, &out.command, &motion_in, &wheel);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        if (i == 0u) { assert(wheel.duty[0] > 0.0f && wheel.duty[1] == 0.0f); }
        if (i == 1u) { assert(wheel.duty[0] == 0.0f && wheel.duty[1] > 0.0f); }
        if (i == 2u) { assert(wheel.duty[0] > 0.0f && wheel.duty[0] == wheel.duty[1]); }
        printf("FOLLOW raw=%u/%u error=%d steer=%d duty L=%.3f R=%.3f PASS\n",
            raw[i][0], raw[i][1], in.line_error, motion_cmd_steering(&out.command),
            wheel.duty[0], wheel.duty[1]);
    }
    assert(mission_queue_direction(&m, BARCODE_LEFT));
    sample(&in, 1u, 1u, 180u);
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_FOLLOW_LINE && m.held == BARCODE_LEFT);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    puts("REQUIREMENT FAIL reproduced: queued LEFT + first both-black sample still FOLLOW, held action unconsumed");
    in.wheel_count[0] = in.wheel_count[1] = MISSION_JUNCTION_MIN_COUNTS;
    sample(&in, 1u, 1u, 200u);
    mission_step(&m, &in, &out);
    assert(m.junction_qualified && m.state == MISSION_FOLLOW_LINE);
    puts("REQUIREMENT FAIL reproduced: qualified both-black still FOLLOW; waits for black exit");
    sample(&in, 0u, 0u, 220u);
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_NAVIGATE && motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    puts("Queued LEFT executes only after leaving both-black");
    mission_init(&m, 240u);
    sample(&in, 1u, 1u, 240u);
    in.start_pressed = true;
    mission_step(&m, &in, &out);
    assert(m.state == MISSION_READY && out.start_refused);
    puts("START BLOCK reproduced: both-black has guidance_valid=false so GP20 refuses start");
    in.start_pressed = false;
    m.state = MISSION_NAVIGATE;
    m.phase = JUNCTION_PHASE_STRAIGHT;
    m.broad_origin[0] = in.wheel_count[0];
    m.broad_origin[1] = in.wheel_count[1];
    sample(&in, 1u, 0u, 260u);
    mission_step(&m, &in, &out);
    assert(in.line_error == -127 && motion_cmd_steering(&out.command) == 0);
    puts("TRACKING SUPPRESSED reproduced: straight junction phase ignores left-black correction for 25 mm");
    return 0;
}
