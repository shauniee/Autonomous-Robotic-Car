#include "ir_tracking.h"
#include <math.h>
#include <stdlib.h>

static float clamp(float value, float lower, float upper)
{
    return value < lower ? lower : (value > upper ? upper : value);
}

static bool raw_valid(IrRaw raw)
{
    return (raw.left <= IR_ADC_MAX) && (raw.right <= IR_ADC_MAX);
}

static bool reading_valid(IrReading reading)
{
    return reading.valid && isfinite(reading.left_fraction) &&
        isfinite(reading.right_fraction) && isfinite(reading.error) &&
        (reading.left_fraction >= 0.0f) && (reading.left_fraction <= 1.0f) &&
        (reading.right_fraction >= 0.0f) && (reading.right_fraction <= 1.0f) &&
        (reading.error >= -1.0f) && (reading.error <= 1.0f);
}

static float fraction(uint16_t raw, uint16_t black, uint16_t white)
{
    /* A signed span supports modules with either electrical polarity. */
    return ((float)raw - (float)white) / ((float)black - (float)white);
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
    reading.left_fraction = clamp(fraction(raw.left, calibration->black.left,
        calibration->white.left), 0.0f, 1.0f);
    reading.right_fraction = clamp(fraction(raw.right, calibration->black.right,
        calibration->white.right), 0.0f, 1.0f);
    reading.error = reading.right_fraction - reading.left_fraction;
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

static IrLine ir_tracking_classify(IrReading reading, IrLine previous)
{
    if (!reading_valid(reading))
    {
        return IR_LINE_UNKNOWN;
    }
    if ((reading.left_fraction > IR_BROAD_BLACK) &&
        (reading.right_fraction > IR_BROAD_BLACK))
    {
        return IR_LINE_BROAD_BLACK;
    }
    if (((previous == IR_LINE_LOST) &&
         (reading.left_fraction < IR_WHITE_EXIT) &&
         (reading.right_fraction < IR_WHITE_EXIT)) ||
        ((reading.left_fraction < IR_WHITE_ENTER) &&
         (reading.right_fraction < IR_WHITE_ENTER)))
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
    tracker->raw = raw;
    tracker->have_sample = true;
    tracker->sample_ms = now;
    tracker->sample_valid = valid;
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
    tracker->line = ir_tracking_classify(tracker->reading, tracker->line);
}

IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now)
{
    IrTrackingOutput output = {0};
    if (tracker == NULL)
    {
        return output;
    }
    output.sampled_ms = tracker->sample_ms;
    bool usable = tracker->have_sample && tracker->sample_valid &&
        reading_valid(tracker->reading) &&
        ((uint32_t)(now - tracker->sample_ms) <= IR_MAX_GAP_MS) &&
        (tracker->capture_kind == IR_CAPTURE_NONE);
    output.line = usable ? tracker->line : IR_LINE_UNKNOWN;
    output.valid = usable && (output.line == IR_LINE_VISIBLE);
    if (output.valid)
    {
        output.error = tracker->reading.error;
        float correction = clamp(IR_KP * output.error,
            -IR_MAX_CORRECTION, IR_MAX_CORRECTION);
        output.steering = correction / IR_MAX_CORRECTION;
    }
    return output;
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
