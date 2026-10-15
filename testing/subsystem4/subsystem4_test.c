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
    c.is_calibrated = true;
    c.height_valid = true;
    c.last_hump_height_m = 0.03f;
    c.max_hump_peak_m = 0.04f;
    terrain_algo_require_calibration(&c);
    TEST_CHECK("S4-U09",
               !c.is_calibrated && !c.height_valid && !c.window_valid &&
                   c.calib_count == 0u && c.max_hump_peak_m == 0.04f &&
                   c.last_hump_height_m == 0.03f,
               "acquisition failure requires calibration and retains results");
    terrain_algo_reinitialize(&c, true);
    bool retained = !c.is_calibrated && c.state == TERRAIN_STATE_FLAT &&
                    c.max_hump_peak_m == 0.04f && c.last_hump_height_m == 0.03f;
    terrain_algo_reinitialize(&c, false);
    TEST_CHECK("S4-U10",
               retained && c.max_hump_peak_m == 0.0f &&
                   c.last_hump_height_m == 0.0f && !c.is_calibrated,
               "reinitialize preserves stop results and clears a new run");
    RobotConfig mapped_cfg = fixture_config();
    mapped_cfg.lateral_sign = -1;
    mapped_cfg.forward_sign = 1;
    mapped_cfg.vertical_sign = -1;
    terrain_algo_init(&c);
    c.is_calibrated = true;
    Gy511RawAxes mapped_raw = {100, 200, -975};
    terrain_algo_process_config(&c, &mapped_raw, 0.01f, true, 20u,
                                &mapped_cfg);
    TEST_CHECK("S4-U11",
               c.pitch_deg > 2.0f && c.pitch_deg < 2.6f &&
                   c.roll_deg > 1.0f && c.roll_deg < 1.4f,
               "axis/sign mapping converts raw milli-g into calibrated pitch and roll");

    S4Calibration identity_cal;
    terrain_calibration_begin(&identity_cal, &c, 41u, 5000u, 0u);
    terrain_calibration_step(&identity_cal, &c, &raw, GY511_NEW, false, 20u);
    bool calibration_lifecycle =
        identity_cal.request_id == 41u &&
        identity_cal.reason == S4_CAL_MOVING &&
        identity_cal.status == S4_CAL_FAILED;
    terrain_calibration_begin(&identity_cal, &c, 42u, 5000u, 0u);
    Gy511RawAxes bad_gravity = {0, 0, 1200};
    terrain_calibration_step(&identity_cal, &c, &bad_gravity, GY511_NEW, true,
                             20u);
    calibration_lifecycle =
        calibration_lifecycle && identity_cal.request_id == 42u &&
        identity_cal.reason == S4_CAL_GRAVITY && !c.is_calibrated;
    terrain_calibration_begin(&identity_cal, &c, 43u, 5000u, 0u);
    terrain_calibration_step(&identity_cal, &c, &raw, GY511_NEW, true, 20u);
    terrain_calibration_cancel(&identity_cal, &c);
    calibration_lifecycle =
        calibration_lifecycle && identity_cal.request_id == 43u &&
        identity_cal.status == S4_CAL_CANCELLED &&
        identity_cal.reason == S4_CAL_STOP && !c.is_calibrated;
    terrain_calibration_begin(&identity_cal, &c, 44u, 5000u, 0u);
    for (unsigned i = 1u; i <= 100u; ++i)
    {
        terrain_calibration_step(&identity_cal, &c, &raw, GY511_NEW, true,
                                 i * 20u);
    }
    TEST_CHECK("S4-U12",
               calibration_lifecycle && identity_cal.request_id == 44u &&
                   identity_cal.status == S4_CAL_DONE && c.is_calibrated,
               "request identity, stationarity/gravity rejection, abort and fresh-request recovery");

    MotionFeedback before = {.valid = true,
                             .sequence = 1u,
                             .sampled_ms = 100u,
                             .run_epoch = 5u,
                             .yaw_deg = 179.0f};
    MotionFeedback after = {.valid = true,
                            .sequence = 2u,
                            .sampled_ms = 160u,
                            .run_epoch = 5u,
                            .yaw_deg = -179.0f,
                            .left_mps = 0.2f};
    bool timing = terrain_motion_measure(&after, &before, true, 160u, &turn,
                                         &acceleration) &&
                  turn > 33.0f && turn < 34.0f;
    MotionFeedback invalid_motion = after;
    invalid_motion.sequence = before.sequence;
    timing = timing && !terrain_motion_measure(&invalid_motion, &before, true,
                                               160u, &turn, &acceleration);
    invalid_motion = after;
    invalid_motion.sampled_ms = before.sampled_ms;
    timing = timing && !terrain_motion_measure(&invalid_motion, &before, true,
                                               160u, &turn, &acceleration);
    invalid_motion = after;
    invalid_motion.run_epoch++;
    timing = timing && !terrain_motion_measure(&invalid_motion, &before, true,
                                               160u, &turn, &acceleration);
    TEST_CHECK("S4-U13", timing,
               "motion derivation wraps yaw and rejects duplicate, zero-dt and cross-epoch samples");

    RobotConfig classify_cfg = fixture_config();
    motion = (MotionFeedback){.valid = true, .stationary = true};
    c.state = TERRAIN_STATE_IMPACT;
    bool priority = terrain_movement_classify(&c, &motion, true, 100, 100,
                                              &classify_cfg) == TERRAIN_IMPACT;
    c.state = TERRAIN_STATE_FLAT;
    priority = priority &&
               terrain_movement_classify(&c, &motion, true, 100, 100,
                                         &classify_cfg) == TERRAIN_STATIONARY;
    motion.stationary = false;
    priority = priority &&
               terrain_movement_classify(&c, &motion, true, 100, 100,
                                         &classify_cfg) == TERRAIN_TURNING;
    c.state = TERRAIN_STATE_CLIMBING;
    priority = priority &&
               terrain_movement_classify(&c, &motion, true, 0, 100,
                                         &classify_cfg) == TERRAIN_CLIMB;
    c.state = TERRAIN_STATE_DESCENDING;
    priority = priority &&
               terrain_movement_classify(&c, &motion, true, 0, 100,
                                         &classify_cfg) == TERRAIN_DESCENT;
    c.state = TERRAIN_STATE_FLAT;
    priority = priority &&
               terrain_movement_classify(&c, &motion, true, 0, 1,
                                         &classify_cfg) == TERRAIN_ACCELERATING &&
               terrain_movement_classify(&c, &motion, false, 0, 0,
                                         &classify_cfg) == TERRAIN_MOVING;
    TEST_CHECK("S4-U14", priority,
               "impact, stationary, turn, terrain phase and acceleration classification priority");

    terrain_algo_init(&c);
    c.is_calibrated = true;
    Gy511RawAxes roll = {300, 0, 954};
    for (unsigned i = 0u; i < 50u; ++i)
    {
        terrain_algo_process_config(&c, &roll, 0.005f, true, i * 20u,
                                    &classify_cfg);
    }
    bool roll_hump = c.state == TERRAIN_STATE_CLIMBING ||
                     c.state == TERRAIN_STATE_CRESTING;
    Gy511RawAxes flat = {0, 0, 1000};
    for (unsigned i = 50u; i < 160u; ++i)
    {
        terrain_algo_process_config(&c, &flat, 0.005f, true, i * 20u,
                                    &classify_cfg);
    }
    float roll_peak = c.max_hump_peak_m;
    terrain_algo_process_config(&c, &flat, -0.01f, true, 3200u,
                                &classify_cfg);
    TEST_CHECK("S4-U15",
               roll_hump && roll_peak > 0.005f && c.height_valid == false &&
                   c.max_hump_peak_m == roll_peak,
               "roll-only hump yields height; negative travel invalidates later window and preserves maximum");

    RunHistory history;
    run_history_reset(&history, 7u);
    bool history_ok = true;
    for (unsigned i = 0u; i < RUN_HISTORY_CAPACITY + 1u; ++i)
    {
        history_ok = run_history_append(
                         &history, (RunEvent){.sampled_at = i, .value = i}) &&
                     history_ok;
    }
    history_ok = history_ok && history.run_epoch == 7u && history.count == 9u &&
                 history.length == RUN_HISTORY_CAPACITY && history.dropped == 1u &&
                 history.events[0].id == 2u && history.events[7].id == 9u;
    history.count = UINT32_MAX;
    history_ok = !run_history_append(&history, (RunEvent){0}) &&
                 history.overflow;
    TerrainSnapshot history_snapshot = {.run_epoch = 7u,
                                        .height_valid = true,
                                        .last_hump_height_m = 0.02f,
                                        .max_hump_peak_m = 0.04f,
                                        .humps = history};
    s4_snapshot_run_reset(&history_snapshot, 8u);
    TEST_CHECK("S4-U16",
               history_ok && history_snapshot.run_epoch == 8u &&
                   history_snapshot.humps.run_epoch == 8u &&
                   history_snapshot.humps.length == 0u &&
                   !history_snapshot.height_valid &&
                   history_snapshot.last_hump_height_m == 0.0f &&
                   history_snapshot.max_hump_peak_m == 0.0f,
               "hump history eviction/overflow is explicit and new-run snapshot resets epoch/history");
    TEST_SUMMARY("subsystem4");
}
#endif
