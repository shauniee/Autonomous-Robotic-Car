#define main existing_motion_suite_main
#include "../../../pure/test_motion_control.c"
#undef main
#include "app_program/mission/mission_logic.h"

static MissionOutputs release(Mission *m, MissionInputs *in)
{
    MissionOutputs out;
    in->now_ms += IR_SAMPLE_MS;
    in->line_sampled_ms = in->now_ms;
    mission_step(m, in, &out);
    return out;
}

static void straight_crossing(void)
{
    Mission m;
    mission_init(&m, 100u);
    m.state = MISSION_FOLLOW_LINE;
    MissionInputs in = {.now_ms = 100u, .inputs_ok = true,
        .usable_line = true, .calibration_valid = true, .mapped = true};
    for (unsigned encounter = 0u; encounter < 2u; ++encounter)
    {
        in.line = IR_LINE_BROAD_BLACK;
        in.guidance_valid = false;
        release(&m, &in);
        in.wheel_count[0] += MISSION_JUNCTION_MIN_COUNTS;
        in.wheel_count[1] += MISSION_JUNCTION_MIN_COUNTS;
        release(&m, &in);
        assert(m.junction_qualified);
        in.line = IR_LINE_LOST;
        MissionOutputs out = release(&m, &in);
        assert(m.state == MISSION_NAVIGATE);
        assert(m.phase == JUNCTION_PHASE_CONFIRM);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        printf("encounter %u entry: NAVIGATE/CONFIRM commands STOP\n", encounter + 1u);
        for (unsigned i = 0; i < 100u; ++i)
        {
            out = release(&m, &in);
            assert(m.state == MISSION_NAVIGATE);
            assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
            assert(motion_cmd_speed(&out.command) == 90u);
            assert(motion_cmd_steering(&out.command) == 0);
            assert(motion_cmd_action(&out.command) == 1u);
        }
        printf("2000 ms LOST: FOLLOW speed=%u steering=%d action=%u; no reacquire\n",
            motion_cmd_speed(&out.command), motion_cmd_steering(&out.command),
            motion_cmd_action(&out.command));
        in.line = IR_LINE_VISIBLE;
        in.guidance_valid = true;
        in.line_error = 100;
        for (unsigned i = 0u; i < MISSION_CONFIRM_SAMPLES; ++i)
        {
            out = release(&m, &in);
            if (i + 1u < MISSION_CONFIRM_SAMPLES)
            {
                assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
                assert(motion_cmd_steering(&out.command) == 0);
            }
        }
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
        puts("third visible sample: FOLLOW_LINE state but STOP command");
        in.line_error = 0;
        out = release(&m, &in);
        assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_speed(&out.command) == 450u);
        puts("next release: FOLLOW speed=450 steering=0 action=1");
    }
}

static void nominal_motion(void)
{
    Sim s;
    sim_init(&s);
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
        90.0f, 0.0f, 0, 1u, s.now);
    float lower = 1.0f, upper = 0.0f;
    unsigned zero = 0u, reverse = 0u;
    for (unsigned i = 0u; i < 300u; ++i)
    {
        MotionOutput out = sim_step(&s, &cmd, false, NULL);
        assert(out.status == RD_MOTION_RUNNING && out.action_id == 1u);
        if (i >= 200u)
        {
            lower = fminf(lower, out.duty[0]);
            upper = fmaxf(upper, out.duty[0]);
            zero += out.duty[0] == 0.0f;
            reverse += out.duty[0] < 0.0f;
        }
    }
    printf("existing idealized plant, final 2 s at 90 mm/s: duty %.6f..%.6f, zero=%u reverse=%u\n",
        (double)lower, (double)upper, zero, reverse);
    assert(zero == 0u && reverse == 0u);
}

int main(void)
{
    straight_crossing();
    nominal_motion();
    puts("INT-59: software behavior assertions PASS; physical jitter not established");
    return 0;
}
