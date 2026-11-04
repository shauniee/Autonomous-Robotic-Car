/* Production-default policy: missing feature release cannot authorize bypass. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/mission/mission_logic.h"

int main(void)
{
    Mission mission;
    mission_init(&mission, 100u);
    MissionInputs in = {.now_ms = 100u, .start_pressed = true,
        .mapped = true, .inputs_ok = true,
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
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_DISTANCE); /* planned slow-down */
    in.motion_action = motion_cmd_action(&out.command);
    in.motion_status = RD_MOTION_DONE;
    in.now_ms += 10u;
    mission_step(&mission, &in, &out);
    in.motion_status = RD_MOTION_IDLE;
    in.stationary = true;
    in.now_ms += 10u;
    mission_step(&mission, &in, &out);
    assert(out.scan_request);
    in.scan_id = out.scan_id;
    in.scan_state = OBSTACLE_SCAN_DONE;
    in.front_sequence++; /* still blocked with the servo back forward */
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
    in.line_error = -IR_SCALE_ONE; /* a touch, then white/white to the loss distance */
    in.left_black = true;
    in.now_ms += 10u;
    in.line_sampled_ms = in.now_ms;
    mission_step(&mission, &in, &out);
    in.line_error = 0;
    in.left_black = false;
    for (unsigned mm = 0u; mm < LINE_LOST_MM; mm += 10u)
    {
        assert(mission.state == MISSION_FOLLOW_LINE && !out.stop_now);
        in.wheel_count[0] = in.wheel_count[1] =
            enc_advance(in.wheel_count[0], (int)ceilf(10.0f * ENC_COUNTS_PER_MM));
        in.now_ms += 10u;
        in.line_sampled_ms = in.now_ms;
        mission_step(&mission, &in, &out);
    }
    assert(mission.state == MISSION_STOPPED && out.line_lost && out.stop_now);
    puts("mission_pending: PASS (150 mm trigger; unreleased bypass stays stopped)");
    return 0;
}
