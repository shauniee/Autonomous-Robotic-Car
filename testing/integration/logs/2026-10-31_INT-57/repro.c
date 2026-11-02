/* Audit fixtures reuse the existing Mission setup, not a second controller. */
#define main existing_mission_suite_main
#include "../../../pure/test_mission.c"
#undef main

static void feedback(MotionController *controller, MissionInputs *in,
    const MissionOutputs *out, MotionOutput *motion)
{
    MotionInput input = {.now_ms = in->now_ms,
        .count = {in->wheel_count[0], in->wheel_count[1]}};
    motion_control_step(controller, &out->command, &input, motion);
    /* Model subsystem2's failure guard; no kernel task is executed. */
    in->motion_stopped = motion->status == RD_MOTION_FAILED;
    in->motion_status = motion->status;
    in->motion_action = motion->action_id;
}

static void wide_heading_handoff(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    MissionOutputs out = enter_line_recovery(&m, &in, &t);
    MotionController controller;
    MotionOutput motion;
    motion_control_reset(&controller);
    feedback(&controller, &in, &out, &motion);
    in.stationary = true;
    out = step(&m, &in, t += MISSION_STATIONARY_MS);
    feedback(&controller, &in, &out, &motion);
    assert(motion.status == RD_MOTION_RUNNING);
    float arc = 80.0f / 57.2957795f * TRACK_WIDTH_MM *
        ENC_COUNTS_PER_MM * 0.5f;
    in.wheel_count[0] = enc_advance(0u, -(int)lroundf(arc));
    in.wheel_count[1] = enc_advance(0u, (int)lroundf(arc));
    in.stationary = false;
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = 100;
    out = step(&m, &in, t += 20u);
    feedback(&controller, &in, &out, &motion);
    assert(motion.status == RD_MOTION_IDLE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    for (unsigned i = 0; i < MISSION_STATIONARY_MS / 10u; ++i)
    {
        out = step(&m, &in, t += 10u);
        feedback(&controller, &in, &out, &motion);
        assert(m.state == MISSION_REACQUIRE);
    }
    in.stationary = true;
    for (unsigned i = 0; i < MISSION_CONFIRM_SAMPLES; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
        feedback(&controller, &in, &out, &motion);
    }
    assert(m.state == MISSION_FOLLOW_LINE);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_STOP);
    out = step(&m, &in, t += 10u);
    feedback(&controller, &in, &out, &motion);
    assert(motion_cmd_mode(&out.command) == MOTION_MODE_FOLLOW);
    assert(motion_cmd_speed(&out.command) > MISSION_FOLLOW_SPEED_MMPS * 0.9f);
    int steering = motion_cmd_steering(&out.command);
    unsigned speed = motion_cmd_speed(&out.command);
    printf("wide handoff: heading=%.2f deg error=%d speed=%u mm/s steering=%d/127 status=%u\n",
        enc_heading_deg(enc_travel_signed(in.wheel_count[0], 0u),
            enc_travel_signed(in.wheel_count[1], 0u)), in.line_error,
        speed, steering, motion.status);
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    in.stationary = false;
    for (unsigned i = 0; i <= MISSION_LINE_LOST_MS / IR_SAMPLE_MS; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
        feedback(&controller, &in, &out, &motion);
        assert(m.state == MISSION_FOLLOW_LINE);
        assert(motion_cmd_speed(&out.command) == speed);
        assert(motion_cmd_steering(&out.command) == steering);
    }
    printf("immediate re-loss: same speed=%u steering=%d retained through %u ms of white/white\n",
        speed, steering, MISSION_LINE_LOST_MS);
    out = step(&m, &in, t += IR_SAMPLE_MS);
    assert(m.state == MISSION_REACQUIRE);
}

static void short_loss_history(void)
{
    Mission m;
    MissionInputs in = base(100u);
    uint32_t t = to_follow(&m, &in);
    MissionOutputs out;
    in.line_error = 64;
    for (unsigned i = 0; i < 150u; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
    }
    int16_t integral = m.line_integral_q;
    in.line = IR_LINE_LOST;
    in.guidance_valid = false;
    for (unsigned i = 0; i < 40u; ++i)
    {
        out = step(&m, &in, t += IR_SAMPLE_MS);
        assert(m.state == MISSION_FOLLOW_LINE);
    }
    in.line = IR_LINE_VISIBLE;
    in.guidance_valid = true;
    in.line_error = -64;
    out = step(&m, &in, t += IR_SAMPLE_MS);
    printf("short loss return: current error=%d steering=%d/127 integral before=%d after=%d primed=%d\n",
        in.line_error, motion_cmd_steering(&out.command), integral,
        m.line_integral_q, m.line_pid_primed);
    if (!LINE_BINARY_RAMP)
    {
        assert(integral > 0 && m.line_integral_q == integral);
        assert(motion_cmd_steering(&out.command) > 0);
    }
    else
    {
        assert(m.line_integral_q == 0);
        assert(motion_cmd_steering(&out.command) < 0);
    }
}

int main(void)
{
    printf("INT-57 audit profile: LINE_BINARY_RAMP=%d\n", LINE_BINARY_RAMP);
    wide_heading_handoff();
    short_loss_history();
    puts("INT-57: software behavior assertions PASS; no physical acceptance");
    return 0;
}
