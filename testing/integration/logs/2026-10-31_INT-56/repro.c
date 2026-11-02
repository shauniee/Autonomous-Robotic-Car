#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "app_program/mission/mission_logic.h"

static MissionOutputs step(Mission *m, MissionInputs *in, uint32_t now)
{
    MissionOutputs out;
    in->now_ms = in->line_sampled_ms = now;
    mission_step(m, in, &out);
    return out;
}

int main(void)
{
    Mission m;
    mission_init(&m, 100u);
    m.state = MISSION_FOLLOW_LINE;
    MissionInputs in = {.inputs_ok = true, .usable_line = true,
        .calibration_valid = true, .mapped = true,
        .line = IR_LINE_BROAD_BLACK};
    step(&m, &in, 100u);
    in.wheel_count[0] = in.wheel_count[1] = MISSION_JUNCTION_MIN_COUNTS;
    step(&m, &in, 120u);
    assert(m.junction_qualified && m.held == BARCODE_UNMAPPED);
    in.line = IR_LINE_LOST;
    step(&m, &in, 140u);
    assert(m.state == MISSION_NAVIGATE &&
        m.phase == JUNCTION_PHASE_CONFIRM);
    MissionOutputs out = {0};
    for (uint32_t now = 160u; now <= 2140u; now += 20u)
    {
        out = step(&m, &in, now);
        assert(m.state == MISSION_NAVIGATE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) == MISSION_BROAD_SPEED_MMPS);
        assert(motion_cmd_steering(&out.command) == 0);
    }
    printf("Unmapped junction + 2000 ms fresh LOST: NAVIGATE/CONFIRM, "
        "mode=%u speed=%u steer=%d action=%u; no recovery\n",
        (unsigned)motion_cmd_mode(&out.command),
        (unsigned)motion_cmd_speed(&out.command),
        (int)motion_cmd_steering(&out.command),
        (unsigned)motion_cmd_action(&out.command));
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 100;
    for (unsigned sample = 0u; sample < MISSION_CONFIRM_SAMPLES; ++sample)
    {
        out = step(&m, &in, 2160u + sample * 20u);
        if (sample + 1u < MISSION_CONFIRM_SAMPLES)
        {
            assert(m.state == MISSION_NAVIGATE);
            assert(motion_cmd_steering(&out.command) == 0);
        }
    }
    assert(m.state == MISSION_FOLLOW_LINE);
    puts("Three distinct VISIBLE samples return to FOLLOW_LINE without barcode");

    MotionController control;
    motion_control_reset(&control);
    MotionInput mi = {.now_ms = 100u};
    MotionCommand cmd = motion_command_make(MOTION_MODE_DISTANCE,
        MISSION_CLEAR_SPEED_MMPS, MISSION_CLEAR_MM, 0, 2u, mi.now_ms);
    MotionOutput mo;
    motion_control_step(&control, &cmd, &mi, &mo);
    int goal = (int)lroundf(MISSION_CLEAR_MM * ENC_COUNTS_PER_MM);
    const int offsets[] = {0, 12, -12, 12};
    mi.count[1] = (uint32_t)(goal - 30);
    for (unsigned i = 0u; i < sizeof offsets / sizeof offsets[0]; ++i)
    {
        mi.count[0] = (uint32_t)(goal + offsets[i]);
        mi.now_ms += MOTION_PERIOD_MS;
        cmd.sampled_ms = mi.now_ms;
        motion_control_step(&control, &cmd, &mi, &mo);
        assert(mo.status == RD_MOTION_RUNNING && mo.action_id == 2u);
        assert(offsets[i] == 0 ? mo.duty[0] == 0.0f :
            (offsets[i] > 0 ? mo.duty[0] < 0.0f : mo.duty[0] > 0.0f));
        printf("Clearance left offset=%d counts, right short=30 counts: "
            "left duty=%.6f action=%u\n", offsets[i],
            (double)mo.duty[0], (unsigned)mo.action_id);
    }
    puts("Scripted true position drift can reactivate/reverse DISTANCE; "
        "not a physical jitter or encoder-noise measurement");
    return 0;
}
