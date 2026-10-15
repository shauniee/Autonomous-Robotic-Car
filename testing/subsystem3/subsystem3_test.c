#include "../../app_program/subsystem3/subsystem3.h"
#include "../../app_program/subsystem3/barcode_decode.h"
#include "../../app_program/test_report.h"
#include "../../app_program/subsystem3/joint_calibration.h"
#include "../../app_program/subsystem3/ir_tracking_calibration.h"
#include "../../app_program/subsystem3/barcode_service.h"
#include "../fixture_config.h"
#include "code39_vectors.h"
#include <math.h>
#include <string.h>
#if APP_TEST
static bool barcode_test(void)
{
    /* Independent printed A/Z patterns from PROJECT_SPECS. */
    const char *patterns[2] = {"nwnnwnwnn"
                               "n"
                               "wnnnnwnnw"
                               "n"
                               "nwnnwnwnn",
                               "nwnnwnwnn"
                               "n"
                               "nwwnwnnnn"
                               "n"
                               "nwnnwnwnn"};
    barcode_config_t cfg = {.left_um_per_count = 10u,
                            .right_um_per_count = 10u,
                            .narrow_min_um = 800u,
                            .narrow_max_um = 1200u,
                            .wide_ratio_min_milli = 2500u,
                            .wide_ratio_max_milli = 3500u,
                            .width_tolerance_milli = 200u,
                            .quiet_zone_um = 3000u,
                            .rearm_um = 3000u,
                            .max_step_um = 5000u,
                            .max_wheel_difference_um = 1000u,
                            .max_frame_travel_um = 100000u,
                            .max_encoder_age_us = 1000u,
                            .max_edge_gap_us = 100000u,
                            .max_frame_us = 2000000u};
    bool pass = true;
    for (unsigned sample = 0u; sample < 2u; ++sample)
    {
        uint32_t widths[29];
        for (unsigned i = 0u; i < 29u; ++i)
        {
            widths[i] = patterns[sample][i] == 'w' ? 3000u : 1000u;
        }
        char symbol = 0;
        barcode_orientation_t orientation;
        pass = pass &&
               barcode_decode(widths, &cfg, &symbol, &orientation) ==
                   BARCODE_DECODED &&
               symbol == (sample == 0u ? 'A' : 'Z');
        for (unsigned i = 0u; i < 14u; ++i)
        {
            uint32_t tmp = widths[i];
            widths[i] = widths[28u - i];
            widths[28u - i] = tmp;
        }
        pass = pass &&
               barcode_decode(widths, &cfg, &symbol, &orientation) ==
                   BARCODE_DECODED &&
               symbol == (sample == 0u ? 'A' : 'Z');
        widths[0] = 0u;
        pass = pass && barcode_decode(widths, &cfg, &symbol, &orientation) !=
                           BARCODE_DECODED;
    }
    return pass;
}
static void alphabet_frame(unsigned letter, unsigned scale, bool reverse,
                           uint32_t widths[29])
{
    for (unsigned i = 0; i < 29u; ++i)
    {
        widths[i] = 1000u * scale;
    }
    const unsigned star[] = {1, 4, 6};
    for (unsigned i = 0; i < 3u; ++i)
    {
        widths[star[i]] *= 3u;
        widths[20u + star[i]] *= 3u;
        widths[10u + code39_wide[letter][i]] *= 3u;
    }
    if (reverse)
    {
        for (unsigned i = 0; i < 14u; ++i)
        {
            uint32_t t = widths[i];
            widths[i] = widths[28u - i];
            widths[28u - i] = t;
        }
    }
}
static bool alphabet_test(void)
{
    RobotConfig cfg = fixture_config();
    bool pass = true;
    uint32_t widths[29];
    for (unsigned letter = 0; letter < 26u; ++letter)
    {
        for (unsigned scale = 1; scale <= 2u; ++scale)
        {
            for (unsigned reversed = 0; reversed < 2u; ++reversed)
            {
                alphabet_frame(letter, scale, reversed != 0u, widths);
                char symbol = 0;
                barcode_orientation_t orientation;
                pass = barcode_decode(widths, &cfg.barcode, &symbol,
                                      &orientation) == BARCODE_DECODED &&
                       symbol == (char)('A' + letter) &&
                       orientation ==
                           (reversed ? BARCODE_REVERSE : BARCODE_FORWARD) &&
                       pass;
                widths[0] = 0;
                pass = barcode_decode(widths, &cfg.barcode, &symbol,
                                      &orientation) != BARCODE_DECODED &&
                       pass;
            }
        }
    }
    return pass;
}
static bool capture_test(void)
{
    RobotConfig cfg = fixture_config();
    barcode_service_t service;
    barcode_service_init(&service);
    run_history_reset(&service.status.encounters, 7u);
    bool pass = barcode_service_configure(&service, &cfg.barcode);
    barcode_action_t map[26] = {BARCODE_LEFT};
    pass = barcode_service_mapping(&service, map) && pass;
    uint32_t widths[29];
    alphabet_frame(0, 1, false, widths);
    barcode_encoder_t e = {.left_count = UINT32_MAX - 200u,
                           .right_count = UINT32_MAX - 200u,
                           .epoch = 3,
                           .valid = true};
    barcode_service_observe(&service, false, &e, 0, false);
    e.left_count += 600u;
    e.right_count += 600u;
    e.sampled_us = 1000;
    barcode_service_observe(&service, true, &e, e.sampled_us, true);
    for (unsigned i = 0; i < 29u; ++i)
    {
        e.left_count += widths[i] / 10u;
        e.right_count += widths[i] / 10u;
        e.sampled_us += 1000;
        barcode_service_observe(&service, (i & 1u) != 0u, &e, e.sampled_us,
                                true);
    }
    e.left_count += 600u;
    e.right_count += 600u;
    e.sampled_us += 1000;
    barcode_service_observe(&service, false, &e, e.sampled_us, false);
    barcode_event_t event;
    pass = pass && barcode_service_pop(&service, &event) &&
           event.symbol == 'A' && event.action == BARCODE_LEFT &&
           !barcode_service_pop(&service, &event);
    barcode_service_reset(&service, BARCODE_CANCELLED);
    return pass && service.status.encounters.count == 1u &&
           service.status.encounters.events[0].value == 'A' &&
           service.status.encounters.run_epoch == 7u;
}
static bool ir_capture_test(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    bool valid = ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u);
    for (uint32_t now = 0u; now <= 500u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){3000u, 500u}, true, now);
    }
    valid = valid && tracker.calibration.black_set &&
            calibration_control_feedback(IR_CAPTURE_BLACK, &tracker) == 1u &&
            ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 520u);
    for (uint32_t now = 520u; now <= 1020u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){500u, 3000u}, true, now);
    }
    valid = valid && tracker.calibration.white_set &&
            calibration_control_feedback(IR_CAPTURE_WHITE, &tracker) == 2u;
    ir_tracking_sample(&tracker, (IrRaw){3000u, 3000u}, true, 1040u);
    IrTrackingOutput output = ir_tracking_output(&tracker, 1040u);
    valid = valid && output.valid && output.steering < 0.0f &&
            !ir_tracking_output(&tracker, 1101u).valid;
    /* A later low-contrast attempt must clear both accepted references. */
    valid = valid && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 1120u);
    for (uint32_t now = 1120u; now <= 1620u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){2000u, 2000u}, true, now);
    }
    valid = valid && ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 1640u);
    for (uint32_t now = 1640u; now <= 2140u; now += IR_SAMPLE_MS)
    {
        ir_tracking_sample(&tracker, (IrRaw){2100u, 2100u}, true, now);
    }
    return valid && tracker.capture_result == IR_CAPTURE_CONTRAST &&
           !tracker.calibration.black_set && !tracker.calibration.white_set;
}

static bool calibration_fault_test(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    bool pass = !ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 100u) &&
                tracker.capture_result == IR_CAPTURE_NEED_BLACK;
    pass = ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, UINT32_MAX - 200u) && pass;
    ir_tracking_sample(&tracker, (IrRaw){1000u, 1100u}, true, UINT32_MAX - 200u);
    uint32_t count = tracker.capture_count;
    ir_tracking_sample(&tracker, (IrRaw){1000u, 1100u}, true, UINT32_MAX - 200u);
    pass = tracker.capture_count == count && pass;
    ir_tracking_check_time(&tracker, 100u);
    pass = tracker.capture_kind == IR_CAPTURE_BLACK && pass;
    ir_tracking_check_time(&tracker, 500u);
    pass = tracker.capture_kind == IR_CAPTURE_NONE &&
           tracker.capture_result == IR_CAPTURE_GAP && pass;

    ir_tracking_init(&tracker);
    pass = ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u) && pass;
    ir_tracking_sample(&tracker, (IrRaw){1200u, 1300u}, false, 20u);
    pass = tracker.capture_kind == IR_CAPTURE_NONE &&
           tracker.capture_result == IR_CAPTURE_INVALID_SAMPLE && pass;
    tracker.calibration = (IrCalibration){.black = {3000u, 3000u},
                                          .white = {500u, 500u},
                                          .black_set = true,
                                          .white_set = true};
    pass = ir_tracking_reading((IrRaw){500u, 3000u}, &tracker.calibration).valid && pass;
    IrReading reverse = ir_tracking_reading((IrRaw){3000u, 500u}, &tracker.calibration);
    return pass && reverse.valid && reverse.error < 0.0f;
}

void subsystem3_test(void)
{
    TEST_INIT();
    IrCalibration cal = {.black = {1000u, 1000u},
                         .white = {3000u, 3000u},
                         .black_set = true,
                         .white_set = true};
    IrReading reading = ir_tracking_reading((IrRaw){1000u, 3000u}, &cal);
    IrTracker guided;
    ir_tracking_init(&guided);
    guided.calibration = cal;
    ir_tracking_sample(&guided, (IrRaw){1000u, 3000u}, true, 100u);
    IrTrackingOutput guidance = ir_tracking_output(&guided, 100u);
    IrTrackingOutput expired = ir_tracking_output(&guided, 161u);
    TEST_CHECK("S3-U06",
               reading.valid && reading.error < 0.0f && guidance.valid &&
                   guidance.steering < 0.0f && guidance.steering >= -1.0f &&
                   !expired.valid && expired.steering == 0.0f,
               "P guidance sign, bound and stale inhibition");
    IrTracker tracker;
    ir_tracking_init(&tracker);
    tracker.calibration = cal;
    ir_tracking_sample(&tracker, (IrRaw){2000u, 2000u}, true, UINT32_MAX - 10u);
    S3IrSnapshot snapshot = ir_snapshot_build(&tracker, 7u, 20u);
    bool fresh = snapshot.normalized_valid && snapshot.sequence == 7u;
    ir_snapshot_refresh(&snapshot, 51u);
    TEST_CHECK("S3-U12",
               fresh && !snapshot.normalized_valid && snapshot.sequence == 7u &&
                   snapshot.sampled_ms == UINT32_MAX - 10u,
               "age across wrap never refreshes acquisition");
    TEST_CHECK("S3-U09", barcode_test(),
               "printed A/Z forward/reverse and invalid element");
    TEST_CHECK(
        "S3-U14",
        joint_calibration_result(false, false, S4_CAL_DONE, 100, 5000) == 0 &&
            joint_calibration_result(true, false, S4_CAL_RUNNING, 100, 5000) ==
                0 &&
            joint_calibration_result(true, false, S4_CAL_DONE, 100, 5000) ==
                1 &&
            joint_calibration_result(true, true, S4_CAL_DONE, 100, 5000) == 3 &&
            joint_calibration_result(true, false, S4_CAL_FAILED, 100, 5000) ==
                3 &&
            joint_calibration_result(true, false, S4_CAL_DONE, 5000, 5000) == 3,
        "one beep only for joint completion; stop/failure/deadline dominates");
    TEST_CHECK("S3-U15", alphabet_test(),
               "A-Z both scales/orientations and zero-width rejection");
    TEST_CHECK("S3-U16", capture_test(),
               "real capture engine, count wrap, single queue event and "
               "retained report");
    calibration_control_t controls;
    calibration_control_init(&controls, 0);
    ir_tracking_init(&tracker);
    tracker.calibration.black_set = true;
    controls.joint_busy = true;
    (void)calibration_handle_buttons(&controls, &tracker, false, false, 31);
    (void)calibration_handle_buttons(&controls, &tracker, true, false, 40);
    (void)calibration_handle_buttons(&controls, &tracker, true, false, 71);
    bool busy = tracker.capture_kind == IR_CAPTURE_NONE;
    (void)calibration_handle_buttons(&controls, &tracker, false, false, 80);
    (void)calibration_handle_buttons(&controls, &tracker, false, false, 111);
    controls.joint_busy = false;
    (void)calibration_handle_buttons(&controls, &tracker, true, false, 120);
    (void)calibration_handle_buttons(&controls, &tracker, true, false, 151);
    busy = busy && tracker.capture_kind == IR_CAPTURE_WHITE;
    unsigned cancelled =
        calibration_handle_buttons(&controls, &tracker, false, true, 160);
    TEST_CHECK(
        "S3-U17",
        busy && cancelled == 3 && tracker.capture_kind == IR_CAPTURE_NONE,
        "busy white press ignored, fresh later press accepted, GP21 cancels");
    TEST_CHECK("S3-U18", ir_capture_test(),
               "black/white capture, opposite polarities, stale guidance and "
               "low-contrast reference reset");
    TEST_CHECK("S3-U19", calibration_fault_test(),
               "IR calibration admission, wrapped timeout, duplicate/invalid samples and polarity");
    TEST_SUMMARY("subsystem3");
}
#endif
