#include "../../app_program/subsystem4/terrain_motion.h"
#include "../../app_program/test_report.h"
#include "../fixture_config.h"
#include <math.h>
#if APP_TEST
void subsystem4_test(void)
{
    TEST_INIT();
    TerrainContext c;
    terrain_algo_init(&c);
    c.max_hump_peak_m = 0.04f;
    c.height_valid = true;
    terrain_algo_invalidate_window(&c);
    TEST_CHECK("S4-U03", !c.height_valid && c.max_hump_peak_m == 0.04f,
               "invalid window preserves accepted peak");
    TerrainSnapshot snapshot = {.max_hump_peak_m = 0.04f, .height_valid = true};
    s4_snapshot_run_reset(&snapshot, 9u);
    terrain_algo_reset_run_peak(&c);
    TEST_CHECK("S4-U04",
               snapshot.run_epoch == 9u && !snapshot.height_valid &&
                   snapshot.max_hump_peak_m == 0 &&
                   snapshot.last_hump_height_m == 0 && c.max_hump_peak_m == 0,
               "new-run snapshot clears before WAIT/error poll");
    Gy511RawAxes raw = {0};
    int16_t axes[3] = {0};
    if (robot_config.vertical_axis < 3u)
    {
        axes[robot_config.vertical_axis] =
            (int16_t)(1000 * robot_config.vertical_sign);
    }
    raw = (Gy511RawAxes){axes[0], axes[1], axes[2]};
    S4Calibration cal;
    terrain_calibration_begin(&cal, &c, 1u, 5000u, 0u);
    terrain_calibration_step(&cal, &c, &raw, GY511_NEW, true, 20u);
    terrain_calibration_step(&cal, &c, &raw, GY511_NEW, true, 20u);
    bool distinct = c.calib_count == 1u;
    for (unsigned i = 2u; i <= 100u; ++i)
    {
        terrain_calibration_step(&cal, &c, &raw, GY511_NEW, true, i * 20u);
    }
    TEST_CHECK("S4-U05",
               distinct && c.is_calibrated && cal.status == S4_CAL_DONE,
               "100 fresh samples; repeated acquisition cannot count twice");
    terrain_calibration_begin(&cal, &c, 2u, 5000u, 0u);
    terrain_calibration_step(&cal, &c, &raw, GY511_NEW, false, 20u);
    bool failures = cal.status == S4_CAL_FAILED && !c.is_calibrated;
    terrain_calibration_begin(&cal, &c, 3u, 5000u, 0u);
    terrain_calibration_step(&cal, &c, &raw, GY511_WAIT, true, 61u);
    failures = failures && cal.reason == S4_CAL_SENSOR;
    terrain_calibration_begin(&cal, &c, 4u, 40u, 0u);
    terrain_calibration_step(&cal, &c, &raw, GY511_NEW, true, 40u);
    failures = failures && cal.reason == S4_CAL_TIMEOUT;
    terrain_calibration_cancel(&cal, &c);
    TEST_CHECK("S4-U06", failures && cal.status == S4_CAL_CANCELLED,
               "movement, stale acquisition, exact deadline and cancellation");
    RobotConfig cfg = fixture_config();
    MotionFeedback previous = {
        .valid = true, .sequence = 1, .sampled_ms = 100, .run_epoch = 1};
    MotionFeedback motion = {.valid = true,
                             .sequence = 2,
                             .sampled_ms = 120,
                             .run_epoch = 1,
                             .yaw_deg = 1,
                             .left_mps = -0.05f,
                             .right_mps = 0.05f};
    float turn, acceleration;
    bool measured = terrain_motion_measure(&motion, &previous, true, 120, &turn,
                                           &acceleration);
    TEST_CHECK("S4-U07",
               measured && turn > 40 &&
                   terrain_movement_classify(&c, &motion, measured, turn,
                                             acceleration,
                                             &cfg) == TERRAIN_TURNING &&
                   !terrain_motion_measure(&motion, &previous, true, 181, &turn,
                                           &acceleration),
               "pivot telemetry independent of straight-travel hump filter");
    terrain_algo_init(&c);
    c.is_calibrated = true;
    Gy511RawAxes uphill = {0, 300, 954}, downhill = {0, -300, 954},
                 level = {0, 0, 1000};
    for (unsigned i = 0; i < 15u; ++i)
    {
        terrain_algo_process_config(&c, &uphill, 0.005f, true, i * 20u, &cfg);
    }
    for (unsigned i = 15; i < 40u; ++i)
    {
        terrain_algo_process_config(&c, &level, 0.005f, true, i * 20u, &cfg);
    }
    for (unsigned i = 40; i < 55u; ++i)
    {
        terrain_algo_process_config(&c, &downhill, 0.005f, true, i * 20u, &cfg);
    }
    for (unsigned i = 55; i < 85u; ++i)
    {
        terrain_algo_process_config(&c, &level, 0.005f, true, i * 20u, &cfg);
    }
    float peak = c.max_hump_peak_m;
    terrain_algo_process_config(&c, &uphill, 0.005f, false, 1700u, &cfg);
    TEST_CHECK("S4-U08",
               peak > 0 && !c.height_valid && c.max_hump_peak_m == peak,
               "valid climb/crest/descent/baseline then invalid-after-valid");
    TEST_SUMMARY("subsystem4");
}
#endif
