#include "ir_tracking.h"
#include <stdlib.h>
#include <stddef.h>

/* Float tunables in thousandths, folded at compile time. */
#define IR_MILLI(x) ((int32_t)((x) * 1000.0f + 0.5f))

static int32_t clamp(int32_t value, int32_t lower, int32_t upper)
{
    return value < lower ? lower : (value > upper ? upper : value);
}

static bool raw_valid(IrRaw raw)
{
    return (raw.left <= IR_ADC_MAX) && (raw.right <= IR_ADC_MAX);
}

static bool reading_valid(IrReading reading)
{
    return reading.valid && (reading.left_fraction <= IR_SCALE_ONE) &&
        (reading.right_fraction <= IR_SCALE_ONE) &&
        (reading.error >= -IR_SCALE_ONE);	/* int8_t tops out at +IR_SCALE_ONE */
}

/* (raw - white) / (black - white) as 0..IR_SCALE_ONE, nearest, clamped. A
   signed span supports modules with either electrical polarity. */
static uint8_t fraction(uint16_t raw, uint16_t black, uint16_t white)
{
    int32_t num = ((int32_t)raw - white) * IR_SCALE_ONE;
    int32_t den = (int32_t)black - white;
    if (den < 0)
    {
        num = -num;
        den = -den;
    }
    if (num <= 0)
    {
        return 0u;
    }
    return (uint8_t)clamp((2 * num + den) / (2 * den), 0, IR_SCALE_ONE);
}

/* Exact comparison of the unrounded fraction with a threshold in thousandths,
   so line states match the real-valued fraction, not its 7-bit rounding. */
static int fraction_cmp(uint16_t raw, uint16_t black, uint16_t white, int32_t milli)
{
    int32_t span = (int32_t)black - white;
    int32_t scaled = ((int32_t)raw - white) * 1000 - milli * span;
    if (span < 0)
    {
        scaled = -scaled;
    }
    return (scaled > 0) - (scaled < 0);
}

IrReading ir_tracking_reading(IrRaw raw, const IrCalibration *calibration)
{
    IrReading reading = {0};
    if ((calibration == NULL) || !calibration->black_set ||
        !calibration->white_set || !raw_valid(raw) ||
        !raw_valid(calibration->black) || !raw_valid(calibration->white))
    {
        return reading;
    }
    int left_span = (int)calibration->black.left - calibration->white.left;
    int right_span = (int)calibration->black.right - calibration->white.right;
    if ((abs(left_span) < (int)IR_MIN_CONTRAST) ||
        (abs(right_span) < (int)IR_MIN_CONTRAST))
    {
        return reading;
    }
    reading.left_fraction = fraction(raw.left, calibration->black.left,
        calibration->white.left);
    reading.right_fraction = fraction(raw.right, calibration->black.right,
        calibration->white.right);
    /* Two-sensor tracking: one sensor on each line edge; centred reads equal. */
    reading.error = (int8_t)(reading.right_fraction - reading.left_fraction);
    reading.valid = true;
    return reading;
}

void ir_tracking_init(IrTracker *tracker)
{
    if (tracker != NULL)
    {
        *tracker = (IrTracker){.capture_result = IR_CAPTURE_WAITING};
    }
}

void ir_tracking_cancel_capture(IrTracker *tracker, IrCaptureResult result)
{
    if ((tracker != NULL) && (tracker->capture_kind != IR_CAPTURE_NONE))
    {
        tracker->capture_result = result;
        tracker->capture_kind = IR_CAPTURE_NONE;
    }
}

bool ir_tracking_capture(IrTracker *tracker, IrCaptureKind kind, uint32_t now)
{
    if (tracker == NULL)
    {
        return false;
    }
    if ((kind != IR_CAPTURE_BLACK) && (kind != IR_CAPTURE_WHITE))
    {
        tracker->capture_result = IR_CAPTURE_UNSUPPORTED;
        return false;
    }
    if (tracker->capture_kind != IR_CAPTURE_NONE)
    {
        tracker->capture_result = IR_CAPTURE_BUSY;
        return false;
    }
    if ((kind == IR_CAPTURE_WHITE) && !tracker->calibration.black_set)
    {
        tracker->capture_result = IR_CAPTURE_NEED_BLACK;
        return false;
    }
    if (kind == IR_CAPTURE_BLACK)
    {
        tracker->calibration = (IrCalibration){0};
    }
    tracker->capture_kind = kind;
    tracker->capture_requested_ms = now;
    tracker->capture_first_ms = 0u;
    tracker->capture_last_ms = 0u;
    tracker->capture_count = 0u;
    tracker->capture_left_sum = 0u;
    tracker->capture_right_sum = 0u;
    tracker->capture_result = IR_CAPTURE_PENDING;
    tracker->reading = (IrReading){0};
    tracker->line = IR_LINE_UNKNOWN;
    return true;
}

void ir_tracking_check_time(IrTracker *tracker, uint32_t now)
{
    if ((tracker == NULL) || (tracker->capture_kind == IR_CAPTURE_NONE))
    {
        return;
    }
    uint32_t last = tracker->capture_count > 0u ? tracker->capture_last_ms :
        tracker->capture_requested_ms;
    if ((uint32_t)(now - last) > IR_MAX_GAP_MS)
    {
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_GAP);
    }
}

static void capture_sample(IrTracker *tracker, IrRaw raw, uint32_t now)
{
    if (tracker->capture_kind == IR_CAPTURE_NONE)
    {
        return;
    }
    /* Bound both sums even for a caller sampling faster than 20 ms. */
    if (tracker->capture_count >= (UINT32_MAX / IR_ADC_MAX))
    {
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_INVALID_SAMPLE);
        return;
    }
    if (tracker->capture_count == 0u)
    {
        tracker->capture_first_ms = now;
    }
    tracker->capture_last_ms = now;
    tracker->capture_left_sum += raw.left;
    tracker->capture_right_sum += raw.right;
    ++tracker->capture_count;
    if (((uint32_t)(now - tracker->capture_first_ms) < IR_CAPTURE_MS) ||
        (tracker->capture_count < IR_CAPTURE_MIN_SAMPLES))
    {
        return;
    }
    /* Averages are bounded by IR_ADC_MAX; narrowing to uint16_t is safe. */
    IrRaw average = {
        .left = (uint16_t)(tracker->capture_left_sum / tracker->capture_count),
        .right = (uint16_t)(tracker->capture_right_sum / tracker->capture_count)
    };
    if (tracker->capture_kind == IR_CAPTURE_BLACK)
    {
        tracker->calibration.black = average;
        tracker->calibration.black_set = true;
        tracker->capture_result = IR_CAPTURE_BLACK_DONE;
    }
    else
    {
        tracker->calibration.white = average;
        tracker->calibration.white_set = true;
        if (ir_tracking_reading(average, &tracker->calibration).valid)
        {
            tracker->capture_result = IR_CAPTURE_WHITE_DONE;
        }
        else
        {
            tracker->calibration = (IrCalibration){0};
            tracker->capture_result = IR_CAPTURE_CONTRAST;
        }
    }
    tracker->capture_kind = IR_CAPTURE_NONE;
}

/* Both sensors against one threshold: sign of each fraction minus it. */
static bool both(IrRaw raw, const IrCalibration *c, int32_t milli, int sign)
{
    return fraction_cmp(raw.left, c->black.left, c->white.left, milli) == sign &&
        fraction_cmp(raw.right, c->black.right, c->white.right, milli) == sign;
}

static IrLine ir_tracking_classify(IrReading reading, IrRaw raw,
    const IrCalibration *calibration, IrLine previous)
{
    if (!reading_valid(reading))
    {
        return IR_LINE_UNKNOWN;
    }
    if (both(raw, calibration, IR_MILLI(IR_BROAD_BLACK), 1))
    {
        return IR_LINE_BROAD_BLACK;
    }
    if (((previous == IR_LINE_LOST) && both(raw, calibration, IR_MILLI(IR_WHITE_EXIT), -1)) ||
        both(raw, calibration, IR_MILLI(IR_WHITE_ENTER), -1))
    {
        return IR_LINE_LOST;
    }
    return IR_LINE_VISIBLE;
}

void ir_tracking_sample(IrTracker *tracker, IrRaw raw, bool acquired,
                        uint32_t now)
{
    if (tracker == NULL)
    {
        return;
    }
    ir_tracking_check_time(tracker, now);
    bool advancing = !tracker->have_sample || (now != tracker->sample_ms);
    bool valid = acquired && raw_valid(raw);
    if (!advancing && valid)
    {
        return; /* Repeated cached samples cannot progress calibration. */
    }
    tracker->raw = ir_raw_word(raw, valid);
    tracker->have_sample = true;
    tracker->sample_ms = now;
    if (!valid)
    {
        tracker->reading = (IrReading){0};
        tracker->line = IR_LINE_UNKNOWN;
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_INVALID_SAMPLE);
        return;
    }
    capture_sample(tracker, raw, now);
    tracker->reading = tracker->capture_kind == IR_CAPTURE_NONE ?
        ir_tracking_reading(raw, &tracker->calibration) : (IrReading){0};
    tracker->line = ir_tracking_classify(tracker->reading, raw,
        &tracker->calibration, tracker->line);
}

IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now)
{
    IrTrackingOutput output = {0};
    if (tracker == NULL)
    {
        return output;
    }
    output.sampled_ms = tracker->sample_ms;
    bool usable = tracker->have_sample && ir_raw_word_valid(tracker->raw) &&
        reading_valid(tracker->reading) &&
        ((uint32_t)(now - tracker->sample_ms) <= IR_MAX_GAP_MS) &&
        (tracker->capture_kind == IR_CAPTURE_NONE);
    output.line = usable ? tracker->line : IR_LINE_UNKNOWN;
    output.valid = usable && (output.line == IR_LINE_VISIBLE);
    if (output.valid)
    {
        output.error = tracker->reading.error;	/* Mission's line PID turns this into steering */
    }
    return output;
}

void ir_snapshot_refresh(S3IrSnapshot *snapshot, uint32_t now)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->sample_age_ms = snapshot->have_sample ?
        (uint32_t)(now - snapshot->sampled_ms) : 0u;
    snapshot->sample_fresh = snapshot->have_sample &&
        ir_raw_word_valid(snapshot->raw) &&
        (snapshot->sample_age_ms <= IR_MAX_GAP_MS);
    if (!snapshot->sample_fresh)
    {
        snapshot->normalized_valid = false;
        snapshot->guidance_valid = false;
        snapshot->left_fraction = 0u;
        snapshot->right_fraction = 0u;
        snapshot->error = 0;
        snapshot->line = IR_LINE_UNKNOWN;
    }
}

S3IrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now)
{
    S3IrSnapshot snapshot = {0};
    if (tracker == NULL)
    {
        return snapshot;
    }
    snapshot.sequence = sequence;
    snapshot.sampled_ms = tracker->sample_ms;
    snapshot.raw = ir_raw_word(ir_raw_word_raw(tracker->raw),
        tracker->have_sample && ir_raw_word_valid(tracker->raw));
    snapshot.calibration = tracker->calibration;
    snapshot.capture_kind = tracker->capture_kind;
    snapshot.capture_result = tracker->capture_result;
    snapshot.have_sample = tracker->have_sample;
    snapshot.calibration_valid =
        ir_tracking_reading(tracker->calibration.black,
                            &tracker->calibration).valid;
    IrReading reading = ir_tracking_reading(ir_raw_word_raw(tracker->raw),
        &tracker->calibration);
    snapshot.normalized_valid = ir_raw_word_valid(snapshot.raw) && reading.valid &&
        (tracker->capture_kind == IR_CAPTURE_NONE);
    if (snapshot.normalized_valid)
    {
        snapshot.left_fraction = reading.left_fraction;
        snapshot.right_fraction = reading.right_fraction;
        snapshot.error = reading.error;
    }
    IrTrackingOutput output = ir_tracking_output(tracker, now);
    snapshot.guidance_valid = output.valid;
    snapshot.line = output.line;
    ir_snapshot_refresh(&snapshot, now);
    return snapshot;
}

#if APP_TEST
const char *ir_tracking_line_name(IrLine line)
{
    switch (line)
    {
        case IR_LINE_VISIBLE: return "visible";
        case IR_LINE_LOST: return "lost";
        case IR_LINE_BROAD_BLACK: return "broad_black";
        default: return "unknown";
    }
}

const char *ir_tracking_capture_name(IrCaptureResult result)
{
    switch (result)
    {
        case IR_CAPTURE_PENDING: return "pending";
        case IR_CAPTURE_BLACK_DONE: return "black_captured";
        case IR_CAPTURE_WHITE_DONE: return "white_captured";
        case IR_CAPTURE_UNSUPPORTED: return "rejected_unsupported";
        case IR_CAPTURE_BUSY: return "rejected_busy";
        case IR_CAPTURE_CANCEL_HELD: return "rejected_cancel_held";
        case IR_CAPTURE_GAP: return "rejected_capture_gap";
        case IR_CAPTURE_INVALID_SAMPLE: return "rejected_invalid_sample";
        case IR_CAPTURE_CONTRAST: return "rejected_contrast";
        case IR_CAPTURE_NEED_BLACK: return "rejected_need_black";
        case IR_CAPTURE_CANCELLED: return "cancelled";
        default: return "waiting";
    }
}
#endif
