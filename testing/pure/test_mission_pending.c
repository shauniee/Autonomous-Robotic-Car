/* Production-default policy: missing feature release cannot authorize bypass. */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/mission/mission_logic.h"

int main(void)
{
    Mission mission;
    mission_init(&mission, 100u);
    MissionInputs in = {.now_ms = 100u, .start_pressed = true,
        .mapped = true, .inputs_ok = true, .calibration_valid = true,
        .guidance_valid = true, .usable_line = true, .line = IR_LINE_VISIBLE,
        .line_sampled_ms = 100u};
    MissionOutputs out;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_FOLLOW_LINE);
    in.start_pressed = false;
    in.front_valid = true;
    in.front_mm = 151u;
    in.now_ms += 10u;
    in.line_sampled_ms = in.now_ms;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_FOLLOW_LINE); /* above the clarified trigger */
    in.front_mm = 150u;
    in.now_ms += 10u;
    in.line_sampled_ms = in.now_ms;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_SCAN);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    in.stationary = true;
    in.now_ms += 10u;
    mission_step(&mission, &in, &out);
    assert(out.scan_request);
    in.scan_id = out.scan_id;
    in.scan_state = S5_SCAN_DONE;
    in.profile_valid = true;
    in.left_clearance_mm = 1000u;
    in.right_clearance_mm = 900u;
    in.now_ms += 10u;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_STOPPED && out.stop_now);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    /* Recovery also stays disabled until physically released. */
    mission_init(&mission, in.now_ms);
    in.front_valid = false;
    in.start_pressed = true;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_FOLLOW_LINE);
    in.start_pressed = false;
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    in.now_ms += 10u;
    in.line_sampled_ms = in.now_ms;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_FOLLOW_LINE && !out.stop_now);
    for (unsigned elapsed = 0u; elapsed < MISSION_LINE_LOST_MS; elapsed += 10u)
    {
        in.now_ms += 10u;
        in.line_sampled_ms = in.now_ms;
        mission_step(&mission, &in, &out);
        assert(mission.state == MISSION_FOLLOW_LINE && !out.stop_now);
    }
    ++in.now_ms;
    in.line_sampled_ms = in.now_ms;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_STOPPED && out.line_lost && out.stop_now);
    puts("mission_pending: PASS (150 mm trigger; unreleased bypass stays stopped)");
    return 0;
}
