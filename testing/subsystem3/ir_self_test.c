#include "ir_self_test.h"
#include "../../app_program/subsystem3/calibration_control.h"
#include "../../app_program/subsystem3/ir_snapshot.h"
#include <math.h>
#include <stddef.h>

#if APP_TEST
static bool near(float actual, float expected)
{
    float difference = actual - expected;
    return isfinite(actual) && (difference > -0.0001f) &&
        (difference < 0.0001f);
}

static IrCalibration calibration(void)
{
    return (IrCalibration){
        .black = {3000u, 1000u},
        .white = {1000u, 3500u},
        .black_set = true,
        .white_set = true
    };
}

bool ir_self_test_normalization(void)
{
    IrCalibration references = calibration();
    IrReading black = ir_tracking_reading(references.black, &references);
    IrReading white = ir_tracking_reading(references.white, &references);
    IrReading middle = ir_tracking_reading((IrRaw){2000u, 2250u}, &references);
    IrReading clamped = ir_tracking_reading((IrRaw){4095u, 4095u}, &references);
    bool pass = black.valid && white.valid && middle.valid && clamped.valid &&
        near(black.left_fraction, 1.0f) && near(black.right_fraction, 1.0f) &&
        near(white.left_fraction, 0.0f) && near(white.right_fraction, 0.0f) &&
        near(middle.left_fraction, 0.5f) && near(middle.right_fraction, 0.5f) &&
        near(clamped.left_fraction, 1.0f) &&
        near(clamped.right_fraction, 0.0f) &&
        !ir_tracking_reading((IrRaw){4096u, 0u}, &references).valid &&
        !ir_tracking_reading((IrRaw){0u, 4096u}, &references).valid &&
        !ir_tracking_reading((IrRaw){0}, NULL).valid;
    references = (IrCalibration){
        .black = {1300u, 1700u}, .white = {1000u, 2000u},
        .black_set = true, .white_set = true
    };
    pass = pass && ir_tracking_reading(references.black, &references).valid;
    --references.black.left;
    pass = pass && !ir_tracking_reading(references.black, &references).valid;
    references.black.left = 1300u;
    ++references.black.right;
    pass = pass && !ir_tracking_reading(references.black, &references).valid;
    references = calibration();
    references.white_set = false;
    return pass && !ir_tracking_reading(references.black, &references).valid;
}

bool ir_self_test_guidance(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    tracker.calibration = calibration();
    ir_tracking_sample(&tracker, (IrRaw){1000u, 1000u}, true, 100u);
    IrTrackingOutput right = ir_tracking_output(&tracker, 160u);
    bool pass = right.valid && near(right.error, 1.0f) &&
        near(right.steering, 1.0f) && (right.sampled_ms == 100u) &&
        !ir_tracking_output(&tracker, 161u).valid;
    ir_tracking_sample(&tracker, (IrRaw){3000u, 3500u}, true, 180u);
    IrTrackingOutput left = ir_tracking_output(&tracker, 180u);
    pass = pass && left.valid && near(left.error, -1.0f) &&
        near(left.steering, -1.0f);
    ir_tracking_sample(&tracker, (IrRaw){2000u, 2250u}, true, 200u);
    IrTrackingOutput equal = ir_tracking_output(&tracker, 200u);
    pass = pass && equal.valid && near(equal.steering, 0.0f);
    uint32_t wrapped = UINT32_MAX - 20u;
    ir_tracking_sample(&tracker, (IrRaw){1000u, 1000u}, true, wrapped);
    pass = pass && ir_tracking_output(&tracker, wrapped + 60u).valid &&
        !ir_tracking_output(&tracker, wrapped + 61u).valid;
    ir_tracking_sample(&tracker, (IrRaw){0}, false, wrapped + 70u);
    return pass && !ir_tracking_output(&tracker, wrapped + 70u).valid;
}

static void collect_capture(IrTracker *tracker, IrRaw raw, uint32_t first)
{
    for (uint32_t sample = 0u; sample <= 25u; ++sample)
    {
        ir_tracking_sample(tracker, raw, true, first + (sample * IR_SAMPLE_MS));
    }
}

bool ir_self_test_capture(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    bool pass = !ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 0u) &&
        (tracker.capture_result == IR_CAPTURE_NEED_BLACK) &&
        !ir_tracking_capture(&tracker, IR_CAPTURE_NONE, 0u) &&
        (tracker.capture_result == IR_CAPTURE_UNSUPPORTED);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u);
    pass = pass && !ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u) &&
        (tracker.capture_result == IR_CAPTURE_BUSY);
    for (uint32_t sample = 0u; sample < 25u; ++sample)
    {
        uint32_t now = 10u + (sample * IR_SAMPLE_MS);
        ir_tracking_sample(&tracker, (IrRaw){3000u, 1000u}, true, now);
        ir_tracking_sample(&tracker, (IrRaw){0}, true, now);
    }
    ir_tracking_check_time(&tracker, 500u);
    pass = pass && (tracker.capture_count == 25u) &&
        (tracker.capture_kind == IR_CAPTURE_BLACK) &&
        !tracker.calibration.black_set;
    ir_tracking_sample(&tracker, (IrRaw){3000u, 1000u}, true, 510u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_BLACK_DONE) &&
        (tracker.calibration.black.left == 3000u) &&
        (tracker.calibration.black.right == 1000u) &&
        (calibration_control_feedback(IR_CAPTURE_BLACK, &tracker) == 1u);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 520u);
    collect_capture(&tracker, (IrRaw){1000u, 3500u}, 530u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_WHITE_DONE) &&
        tracker.calibration.white_set &&
        (calibration_control_feedback(IR_CAPTURE_WHITE, &tracker) == 2u);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 1040u) &&
        !tracker.calibration.black_set && !tracker.calibration.white_set;
    ir_tracking_cancel_capture(&tracker, IR_CAPTURE_CANCELLED);
    pass = pass && !tracker.calibration.black_set;
    tracker.calibration = calibration();
    tracker.calibration.white_set = false;
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 1100u);
    ir_tracking_cancel_capture(&tracker, IR_CAPTURE_CANCELLED);
    pass = pass && tracker.calibration.black_set &&
        !tracker.calibration.white_set;
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 1120u);
    collect_capture(&tracker, (IrRaw){2701u, 1299u}, 1130u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_CONTRAST) &&
        !tracker.calibration.black_set && !tracker.calibration.white_set;

    ir_tracking_init(&tracker);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u);
    for (uint32_t sample = 0u; sample < 13u; ++sample)
    {
        ir_tracking_sample(&tracker, (IrRaw){2000u, 2000u}, true, sample * 50u);
    }
    pass = pass && (tracker.capture_kind == IR_CAPTURE_BLACK) &&
        (tracker.capture_count == 13u); /* Duration alone is insufficient. */
    ir_tracking_check_time(&tracker, 660u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_BLACK);
    ir_tracking_check_time(&tracker, 661u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_GAP);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 700u);
    ir_tracking_sample(&tracker, (IrRaw){0}, false, 710u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_INVALID_SAMPLE);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 720u);
    ir_tracking_sample(&tracker, (IrRaw){4096u, 1u}, true, 730u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_INVALID_SAMPLE);

    ir_tracking_init(&tracker);
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, 0u);
    ir_tracking_check_time(&tracker, 61u);
    pass = pass && (tracker.capture_result == IR_CAPTURE_GAP);
    uint32_t first = UINT32_MAX - 100u;
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_BLACK, first);
    collect_capture(&tracker, (IrRaw){3000u, 1000u}, first + 10u);
    return pass && (tracker.capture_result == IR_CAPTURE_BLACK_DONE);
}

static IrReading reading(float left, float right)
{
    return (IrReading){.valid = true, .left_fraction = left,
        .right_fraction = right, .error = right - left};
}

bool ir_self_test_classification(void)
{
    const float bands[] = {0.0f, 0.5f, 1.0f};
    bool pass = true;
    for (unsigned left = 0u; left < 3u; ++left)
    {
        for (unsigned right = 0u; right < 3u; ++right)
        {
            IrLine expected = IR_LINE_VISIBLE;
            if ((left == 0u) && (right == 0u))
            {
                expected = IR_LINE_LOST;
            }
            else if ((left == 2u) && (right == 2u))
            {
                expected = IR_LINE_BROAD_BLACK;
            }
            IrReading sample = reading(bands[left], bands[right]);
            pass = pass &&
                (ir_tracking_classify(sample, IR_LINE_UNKNOWN) == expected);
            if (left != right)
            {
                pass = pass && ((sample.error > 0.0f) == (right > left));
            }
        }
    }
    return pass &&
        (ir_tracking_classify(reading(0.15f, 0.15f), IR_LINE_UNKNOWN) ==
            IR_LINE_VISIBLE) &&
        (ir_tracking_classify(reading(0.24f, 0.24f), IR_LINE_LOST) ==
            IR_LINE_LOST) &&
        (ir_tracking_classify(reading(0.25f, 0.0f), IR_LINE_LOST) ==
            IR_LINE_VISIBLE) &&
        (ir_tracking_classify(reading(0.85f, 0.85f), IR_LINE_UNKNOWN) ==
            IR_LINE_VISIBLE) &&
        (ir_tracking_classify(reading(0.86f, 0.86f), IR_LINE_UNKNOWN) ==
            IR_LINE_BROAD_BLACK) &&
        (ir_tracking_classify(reading(NAN, 0.5f), IR_LINE_UNKNOWN) ==
            IR_LINE_UNKNOWN) &&
        (ir_tracking_classify(reading(INFINITY, 0.5f), IR_LINE_UNKNOWN) ==
            IR_LINE_UNKNOWN);
}

static unsigned button(calibration_control_t *control, IrTracker *tracker,
    bool down, bool cancel, uint32_t now)
{
    return calibration_control_inputs(control, tracker, down, cancel, now);
}

bool ir_self_test_buttons(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    calibration_control_t control;
    calibration_control_init(&control);
    button(&control, &tracker, true, false, 0u);
    button(&control, &tracker, true, false, 30u);
    bool pass = tracker.capture_kind == IR_CAPTURE_NONE; /* Held at boot. */
    button(&control, &tracker, false, false, 40u);
    button(&control, &tracker, false, false, 70u);
    button(&control, &tracker, true, false, 80u);
    button(&control, &tracker, false, false, 90u); /* Bounce. */
    button(&control, &tracker, true, false, 100u);
    button(&control, &tracker, true, false, 129u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_NONE);
    button(&control, &tracker, true, false, 130u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_BLACK);
    collect_capture(&tracker, (IrRaw){3000u, 1000u}, 140u);
    button(&control, &tracker, true, false, 650u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_NONE);
    button(&control, &tracker, false, false, 660u);
    button(&control, &tracker, false, false, 690u);
    button(&control, &tracker, true, false, 700u);
    button(&control, &tracker, true, false, 730u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_WHITE);
    pass = pass && (button(&control, &tracker, true, true, 740u) == 3u) &&
        (tracker.capture_kind == IR_CAPTURE_NONE) &&
        tracker.calibration.black_set;
    button(&control, &tracker, false, true, 750u);
    button(&control, &tracker, false, true, 780u);
    button(&control, &tracker, true, true, 790u);
    pass = pass && (button(&control, &tracker, true, true, 820u) == 3u) &&
        (tracker.capture_result == IR_CAPTURE_CANCEL_HELD);
    button(&control, &tracker, true, false, 830u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_NONE);
    button(&control, &tracker, false, false, 840u);
    button(&control, &tracker, false, false, 870u);
    button(&control, &tracker, true, false, 880u);
    button(&control, &tracker, true, false, 910u);
    collect_capture(&tracker, (IrRaw){1000u, 3500u}, 920u);
    pass = pass && tracker.calibration.white_set;
    button(&control, &tracker, false, false, 1430u);
    button(&control, &tracker, false, false, 1460u);
    button(&control, &tracker, true, false, 1470u);
    button(&control, &tracker, true, false, 1500u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_BLACK) &&
        !tracker.calibration.black_set && !tracker.calibration.white_set;
    ir_tracking_cancel_capture(&tracker, IR_CAPTURE_CANCELLED);
    calibration_control_init(&control);
    /* Do not replay a press begun under cancel after an early release. */
    button(&control, &tracker, false, false, 1510u);
    button(&control, &tracker, false, false, 1540u);
    button(&control, &tracker, true, true, 1550u);
    button(&control, &tracker, true, false, 1560u);
    button(&control, &tracker, true, false, 1580u);
    pass = pass && (tracker.capture_kind == IR_CAPTURE_NONE);
    calibration_control_init(&control);
    uint32_t wrapped = UINT32_MAX - 20u;
    button(&control, &tracker, false, false, wrapped);
    button(&control, &tracker, true, false, wrapped + 10u);
    button(&control, &tracker, true, false, wrapped + 40u);
    return pass && (tracker.capture_kind == IR_CAPTURE_BLACK);
}

bool ir_self_test_snapshot(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    S3IrSnapshot snapshot = ir_snapshot_build(&tracker, 0u, 0u);
    bool pass = !snapshot.have_sample && !snapshot.acquisition_valid &&
        !snapshot.normalized_valid && !snapshot.guidance_valid;
    ir_tracking_sample(&tracker, (IrRaw){2000u, 2250u}, true, 100u);
    snapshot = ir_snapshot_build(&tracker, 1u, 100u);
    pass = pass && snapshot.acquisition_valid && snapshot.sample_fresh &&
        !snapshot.calibration_valid && !snapshot.normalized_valid;
    tracker.calibration = calibration();
    ir_tracking_sample(&tracker, tracker.calibration.black, true, 120u);
    snapshot = ir_snapshot_build(&tracker, 2u, 120u);
    pass = pass && snapshot.normalized_valid && !snapshot.guidance_valid &&
        (snapshot.line == IR_LINE_BROAD_BLACK) &&
        near(snapshot.normalized_left, 1.0f) &&
        near(snapshot.normalized_right, 1.0f) &&
        (snapshot.contrast.left == 2000u) && (snapshot.contrast.right == 2500u);
    ir_tracking_sample(&tracker, tracker.calibration.white, true, 140u);
    snapshot = ir_snapshot_build(&tracker, 3u, 140u);
    pass = pass && snapshot.normalized_valid && !snapshot.guidance_valid &&
        (snapshot.line == IR_LINE_LOST) &&
        near(snapshot.normalized_left, -1.0f) &&
        near(snapshot.normalized_right, -1.0f);
    ir_tracking_sample(&tracker, (IrRaw){1000u, 1000u}, true, 160u);
    snapshot = ir_snapshot_build(&tracker, 4u, 160u);
    pass = pass && snapshot.guidance_valid && near(snapshot.error, 1.0f) &&
        near(snapshot.steering, 1.0f) && (snapshot.raw.left == 1000u);
    ir_snapshot_refresh(&snapshot, 220u);
    pass = pass && snapshot.guidance_valid && (snapshot.sample_age_ms == 60u) &&
        (snapshot.sampled_ms == 160u) && (snapshot.sequence == 4u);
    ir_snapshot_refresh(&snapshot, 221u);
    pass = pass && !snapshot.normalized_valid && !snapshot.guidance_valid &&
        snapshot.acquisition_valid && snapshot.calibration_valid &&
        (snapshot.line == IR_LINE_UNKNOWN) && (snapshot.sample_age_ms == 61u) &&
        (snapshot.sampled_ms == 160u) && (snapshot.sequence == 4u);
    ir_tracking_sample(&tracker, (IrRaw){0}, false, 240u);
    snapshot = ir_snapshot_build(&tracker, 5u, 240u);
    pass = pass && snapshot.have_sample && !snapshot.acquisition_valid &&
        !snapshot.normalized_valid && snapshot.calibration_valid;
    pass = pass && ir_tracking_capture(&tracker, IR_CAPTURE_WHITE, 250u);
    ir_tracking_sample(&tracker, (IrRaw){2000u, 2250u}, true, 260u);
    snapshot = ir_snapshot_build(&tracker, 6u, 260u);
    pass = pass && snapshot.sample_fresh && !snapshot.normalized_valid &&
        !snapshot.guidance_valid && (snapshot.capture_kind == IR_CAPTURE_WHITE);
    ir_snapshot_refresh(NULL, 0u);
    return pass && !ir_snapshot_build(NULL, 0u, 0u).have_sample;
}

#endif
