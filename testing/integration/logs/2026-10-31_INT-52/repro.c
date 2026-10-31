/* Host-only audit reproduction: intentional recovery brake trips Motion. */
#include <assert.h>
#include <stdio.h>
#include "app_program/mission/mission_logic.h"

int main(void)
{
    Mission mission;
    mission_init(&mission, 100u);
    mission.state = MISSION_REACQUIRE;
    MissionInputs in = {.now_ms = 100u, .line_sampled_ms = 100u,
        .inputs_ok = true, .usable_line = true, .calibration_valid = true,
        .line = IR_LINE_LOST, .stationary = true};
    recovery_input_t initial = {.now_ms = 100u, .sampled_ms = 100u};
    recovery_control_begin(&mission.recovery, &initial);
    MissionOutputs out;
    mission_step(&mission, &in, &out);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_TURN);
    MotionController controller;
    motion_control_reset(&controller);
    MotionInput motion_in = {.now_ms = 100u};
    MotionOutput motion_out;
    motion_control_step(&controller, &out.command, &motion_in, &motion_out);
    assert(motion_out.status == RD_MOTION_RUNNING);
    puts("Recovery sweep: TURN -> Motion RUNNING");

    in.now_ms = in.line_sampled_ms = motion_in.now_ms = 120u;
    in.stationary = false;
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 80;
    in.motion_status = motion_out.status;
    in.motion_action = motion_out.action_id;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_REACQUIRE);
    assert(mission.recovery.phase == RECOVERY_ALIGN);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    motion_control_step(&controller, &out.command, &motion_in, &motion_out);
    assert(motion_out.status == RD_MOTION_FAILED);
    puts("Line detected: ALIGN requests STOP -> Motion FAILED (bug reproduced)");

    /* subsystem2.c latches STOP on RD_MOTION_FAILED; model that health input. */
    in.motion_stopped = true;
    in.motion_status = motion_out.status;
    in.now_ms = in.line_sampled_ms = 140u;
    mission_step(&mission, &in, &out);
    assert(mission.state == MISSION_STOPPED);
    puts("Motion stop latch -> Mission STOPPED; alignment cannot continue");
    return 0;
}
