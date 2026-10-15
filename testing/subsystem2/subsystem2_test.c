#include "../../app_program/subsystem2/subsystem2.h"
#include "../../app_program/test_report.h"
#if APP_TEST
void subsystem2_test(void)
{
    TEST_INIT();
    bool encoder = drv_encoder_transition(0u, 1u) == 1 &&
                   drv_encoder_transition(1u, 3u) == 1 &&
                   drv_encoder_transition(3u, 2u) == 1 &&
                   drv_encoder_transition(2u, 0u) == 1 &&
                   drv_encoder_transition(0u, 2u) == -1 &&
                   drv_encoder_transition(0u, 3u) == 2;
    TEST_CHECK("S2-U06", encoder,
               "quadrature direction and illegal transition");
    RobotConfig cfg = {
        .speed_max_mps = 0.2f,
        .track_m = 0.1f,
        .distance_tolerance_m = 0.001f,
        .angle_tolerance_deg = 1.0f,
        .stationary_mps = 0.001f,
        .stall_ms = 500u,
        .action_ms = 2000u,
        .kp = 1.0f}; /* Synthetic fixture, not measured hardware settings. */
    MotionController control;
    motion_control_init(&control, 7u);
    MotionCommand cmd = {.action_id = 1u,
                         .run_epoch = 7u,
                         .mode = MOTION_DISTANCE,
                         .target = 0.1f,
                         .speed_mps = 0.1f,
                         .sampled_ms = 100u,
                         .issued_ms = 100u,
                         .expires_ms = 160u,
                         .deadline_ms = 1100u};
    bool action = motion_control_accept(&control, &cmd, 0.0f, 0.0f, 100u, &cfg);
    cmd.issued_ms = 120u;
    cmd.expires_ms = 180u;
    cmd.sampled_ms = 120u;
    action = action &&
             motion_control_accept(&control, &cmd, 0.04f, 0.04f, 120u, &cfg) &&
             control.origin_left == 0.0f &&
             control.command.deadline_ms == 1100u;
    float left, right;
    motion_control_step(&control, 0.1f, 0.05f, 0.0f, 0.0f, 0.02f, 139u, &cfg,
                        &left, &right);
    action = action && control.status == MOTION_ACTIVE && left == 0.0f &&
             right > 0.0f;
    motion_control_step(&control, 0.1f, 0.1f, 0.0f, 0.0f, 0.02f, 140u, &cfg,
                        &left, &right);
    action = action && control.status == MOTION_DONE && left == 0.0f &&
             right == 0.0f && motion_control_ack(&control, 1u);
    TEST_CHECK(
        "S2-U07", action,
        "renewal preserves origin/deadline; completion requires both wheels");
    cmd.run_epoch = 6u;
    TEST_CHECK("S2-U08", !motion_command_valid(&cmd, 140u, 7u, &cfg),
               "old run command cannot replay after reset");
    RobotConfig bench = {.electrical_confirmed = true,
                         .commissioning = COMMISSIONING_DUTY};
    MotionCommand pulse = {.action_id = 1u,
                           .run_epoch = 1u,
                           .mode = MOTION_DUTY_BENCH,
                           .issued_ms = 100u,
                           .sampled_ms = 100u,
                           .expires_ms = 160u,
                           .deadline_ms = 2100u,
                           .left_duty = 0.1f,
                           .right_duty = 0.1f};
    bool commissioning = motion_command_valid(&pulse, 100u, 1u, &bench);
    bench.electrical_confirmed = false;
    TEST_CHECK(
        "S2-U09",
        commissioning && !motion_command_valid(&pulse, 100u, 1u, &bench),
        "bounded local commissioning requires explicit electrical release");
    motion_control_init(&control, 7u);
    cmd.run_epoch = 7u;
    cmd.action_id = 2u;
    cmd.sampled_ms = cmd.issued_ms = 100u;
    cmd.expires_ms = 160u;
    bool expired_action =
        motion_control_accept(&control, &cmd, 0, 0, 100u, &cfg);
    motion_control_step(&control, 0, 0, 0, 0, 0.02f, 160u, &cfg, &left, &right);
    TEST_CHECK("S2-U10",
               expired_action && control.status == MOTION_FAILED &&
                   control.reason == MOTION_EXPIRED && left == 0 && right == 0,
               "exact expiry brakes; no one-wheel premature completion");
    cmd.sampled_ms = cmd.issued_ms = UINT32_MAX - 20u;
    cmd.expires_ms = 39u;
    cmd.deadline_ms = 999u;
    TEST_CHECK("S2-U11",
               motion_command_valid(&cmd, 10u, 7u, &cfg) &&
                   !motion_command_valid(&cmd, 39u, 7u, &cfg),
               "lease wrap and deadline boundary");
    bool signed_actions = true;
    const float angles[] = {90, -90, 180};
    const float right_travel[] = {0.0785398f, -0.0785398f, 0.1570796f};
    for (unsigned i = 0; i < 3u; ++i)
    {
        motion_control_init(&control, 7);
        cmd = (MotionCommand){.action_id = 1,
                              .run_epoch = 7,
                              .mode = MOTION_TURN,
                              .target = angles[i],
                              .speed_mps = 0.1f,
                              .issued_ms = 100,
                              .sampled_ms = 100,
                              .expires_ms = 160,
                              .deadline_ms = 1100};
        signed_actions =
            motion_control_accept(&control, &cmd, 0, 0, 100, &cfg) &&
            signed_actions;
        motion_control_step(&control, -right_travel[i], right_travel[i], 0, 0,
                            0.02f, 140, &cfg, &left, &right);
        signed_actions = signed_actions && control.status == MOTION_DONE &&
                         left == 0 && right == 0;
    }
    TEST_CHECK("S2-U12", signed_actions,
               "left/right/U-turn use signed encoder travel");
    motion_control_init(&control, 7);
    cmd = (MotionCommand){.action_id = 1,
                          .run_epoch = 7,
                          .mode = MOTION_SPEED,
                          .left_mps = -0.1f,
                          .right_mps = 0.1f,
                          .issued_ms = 100,
                          .sampled_ms = 100,
                          .expires_ms = 160,
                          .deadline_ms = 1100};
    bool pid = motion_control_accept(&control, &cmd, 0, 0, 100, &cfg);
    motion_control_step(&control, 0, 0, 0, 0, 0.02f, 140, &cfg, &left, &right);
    TEST_CHECK("S2-U13",
               pid && left < 0 && right > 0 && left >= -MOTION_MAX_DUTY &&
                   right <= MOTION_MAX_DUTY,
               "signed wheel PID respects independent output bounds");
    TEST_SUMMARY("subsystem2");
}
#endif
