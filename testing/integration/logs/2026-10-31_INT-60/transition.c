#define main existing_motion_suite_main
#include "../../../pure/test_motion_control.c"
#undef main

int main(void)
{
    for (unsigned stopped = 0u; stopped < 2u; ++stopped)
    {
        Sim s;
        sim_init(&s);
        MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
            450.0f, 0.0f, 0, 1u, s.now);
        for (unsigned i = 0u; i < 300u; ++i)
        {
            sim_step(&s, &cmd, false, NULL);
        }
        if (stopped)
        {
            cmd = motion_command_make(MOTION_MODE_STOP, 0.0f, 0.0f,
                0, 0u, s.now);
            MotionOutput out = sim_step(&s, &cmd, false, NULL);
            assert(out.duty[0] == 0.0f && out.status == RD_MOTION_IDLE);
        }
        cmd = motion_command_make(MOTION_MODE_FOLLOW, 90.0f,
            0.0f, 0, 1u, s.now);
        unsigned brake[2] = {0u, 0u}, reverse = 0u;
        float lower = 1.0f, upper = 0.0f;
        for (unsigned i = 0u; i < 300u; ++i)
        {
            MotionOutput out = sim_step(&s, &cmd, false, NULL);
            assert(out.status == RD_MOTION_RUNNING);
            brake[i < 100u ? 0u : 1u] += out.duty[0] == 0.0f;
            reverse += out.duty[0] < 0.0f;
            if (i >= 200u)
            {
                lower = fminf(lower, out.duty[0]);
                upper = fmaxf(upper, out.duty[0]);
            }
        }
        printf("450 -> %s90: braked first 2s=%u later 4s=%u reversed=%u final duty=%.6f..%.6f\n",
            stopped ? "STOP -> " : "", brake[0], brake[1], reverse,
            (double)lower, (double)upper);
        assert(reverse == 0u && brake[1] == 0u);
    }
    puts("INT-60: idealized transition assertions PASS; no physical friction claim");
    return 0;
}
