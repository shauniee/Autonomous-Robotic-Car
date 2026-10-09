#include "ir_tracking.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static float clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static bool raw_valid(IrRaw raw) { return raw.left <= 4095 && raw.right <= 4095; }
static bool reading_valid(IrReading r) {
    return r.valid && isfinite(r.left_fraction) && isfinite(r.right_fraction) &&
        isfinite(r.error) && r.left_fraction >= 0 && r.left_fraction <= 1 &&
        r.right_fraction >= 0 && r.right_fraction <= 1 && r.error >= -1 && r.error <= 1;
}
static float fraction(uint16_t raw, uint16_t black, uint16_t white) {
    return ((float)raw - white) / ((int)black - (int)white);
}
IrReading ir_tracking_reading(IrRaw raw, const IrCalibration *c) {
    IrReading r = {0};
    if (!c->black_set || !c->white_set || !raw_valid(raw) ||
        !raw_valid(c->black) || !raw_valid(c->white) ||
        abs((int)c->black.left - c->white.left) < IR_MIN_CONTRAST ||
        abs((int)c->black.right - c->white.right) < IR_MIN_CONTRAST) return r;
    r.valid = true;
    r.left_fraction = clamp(fraction(raw.left, c->black.left, c->white.left), 0, 1);
    r.right_fraction = clamp(fraction(raw.right, c->black.right, c->white.right), 0, 1);
    r.error = r.right_fraction - r.left_fraction;
    return r;
}
void ir_tracking_init(IrTracker *f) {
    memset(f, 0, sizeof(*f));
    f->capture_result = IR_CAPTURE_WAITING;
}
void ir_tracking_cancel_capture(IrTracker *f, IrCaptureResult result) {
    if (f->capture_kind == IR_CAPTURE_NONE) return;
    f->capture_result = result; f->capture_kind = IR_CAPTURE_NONE;
}
bool ir_tracking_capture(IrTracker *f, IrCaptureKind kind, uint32_t now) {
    if (kind != IR_CAPTURE_BLACK && kind != IR_CAPTURE_WHITE) {
        f->capture_result = IR_CAPTURE_UNSUPPORTED; return false;
    }
    if (f->capture_kind != IR_CAPTURE_NONE) {
        f->capture_result = IR_CAPTURE_BUSY; return false;
    }
    if (kind == IR_CAPTURE_WHITE && !f->calibration.black_set) {
        f->capture_result = IR_CAPTURE_NEED_BLACK; return false;
    }
    if (kind == IR_CAPTURE_BLACK) f->calibration = (IrCalibration){0};
    f->capture_kind = kind; f->capture_ms = now;
    f->capture_count = f->capture_left = f->capture_right = 0;
    f->capture_result = IR_CAPTURE_PENDING;
    f->reading = (IrReading){0}; f->line = IR_LINE_UNKNOWN;
    return true;
}
void ir_tracking_check_time(IrTracker *f, uint32_t now) {
    uint32_t last = f->have_sample ? f->sample_ms : f->capture_ms;
    if ((uint32_t)(now - last) > IR_MAX_GAP_MS)
        ir_tracking_cancel_capture(f, IR_CAPTURE_GAP);
}
static void capture_sample(IrTracker *f, IrRaw raw, uint32_t now) {
    if (f->capture_kind == IR_CAPTURE_NONE) return;
    f->capture_left += raw.left; f->capture_right += raw.right; ++f->capture_count;
    if ((uint32_t)(now - f->capture_ms) < IR_CAPTURE_MS ||
        f->capture_count < IR_CAPTURE_MS / IR_SAMPLE_MS) return;
    IrRaw average = {(uint16_t)(f->capture_left / f->capture_count),
                     (uint16_t)(f->capture_right / f->capture_count)};
    if (f->capture_kind == IR_CAPTURE_BLACK) {
        f->calibration.black = average; f->calibration.black_set = true;
        f->capture_result = IR_CAPTURE_BLACK_DONE;
    } else {
        f->calibration.white = average; f->calibration.white_set = true;
        if (ir_tracking_reading(average, &f->calibration).valid) {
            f->capture_result = IR_CAPTURE_WHITE_DONE;
        } else {
            f->calibration = (IrCalibration){0};
            f->capture_result = IR_CAPTURE_CONTRAST;
        }
    }
    f->capture_kind = IR_CAPTURE_NONE;
}
IrLine ir_tracking_classify(IrReading r, IrLine previous) {
    if (!reading_valid(r)) return IR_LINE_UNKNOWN;
    if (r.left_fraction > IR_BROAD_BLACK && r.right_fraction > IR_BROAD_BLACK)
        return IR_LINE_BROAD_BLACK;
    if ((previous == IR_LINE_LOST && r.left_fraction < IR_WHITE_EXIT &&
         r.right_fraction < IR_WHITE_EXIT) ||
        (r.left_fraction < IR_WHITE_ENTER && r.right_fraction < IR_WHITE_ENTER))
        return IR_LINE_LOST;
    return IR_LINE_VISIBLE;
}
void ir_tracking_sample(IrTracker *f, IrRaw raw, bool acquired, uint32_t now) {
    ir_tracking_check_time(f, now); /* Examine old age before refreshing it. */
    bool advancing = !f->have_sample || now != f->sample_ms;
    f->raw = raw; f->have_sample = true; f->sample_ms = now;
    f->sample_valid = acquired && raw_valid(raw);
    if (!f->sample_valid) {
        f->reading = (IrReading){0}; f->line = IR_LINE_UNKNOWN;
        ir_tracking_cancel_capture(f, IR_CAPTURE_INVALID_SAMPLE);
        return;
    }
    if (advancing) capture_sample(f, raw, now);
    f->reading = ir_tracking_reading(raw, &f->calibration);
    f->line = ir_tracking_classify(f->reading, f->line);
}
IrTrackingOutput ir_tracking_output(const IrTracker *f, uint32_t now) {
    bool fresh = f->have_sample && f->sample_valid && reading_valid(f->reading) &&
        (uint32_t)(now - f->sample_ms) <= IR_MAX_GAP_MS;
    bool valid = fresh && f->line == IR_LINE_VISIBLE && f->capture_kind == IR_CAPTURE_NONE;
    float correction = valid ? clamp(IR_KP * f->reading.error,
                                     -IR_MAX_CORRECTION, IR_MAX_CORRECTION) : 0;
    return (IrTrackingOutput){.valid = valid, .line = fresh ? f->line : IR_LINE_UNKNOWN,
        .error = valid ? f->reading.error : 0, .steering = correction / IR_MAX_CORRECTION,
        .sampled_ms = f->sample_ms};
}
const char *ir_tracking_line_name(IrLine line) {
    switch (line) {
        case IR_LINE_VISIBLE: return "visible";
        case IR_LINE_LOST: return "lost";
        case IR_LINE_BROAD_BLACK: return "broad_black";
        default: return "unknown";
    }
}
const char *ir_tracking_capture_name(IrCaptureResult result) {
    switch (result) {
        case IR_CAPTURE_PENDING: return "pending";
        case IR_CAPTURE_BLACK_DONE: return "black_captured";
        case IR_CAPTURE_WHITE_DONE: return "white_captured";
        case IR_CAPTURE_UNSUPPORTED: return "rejected_unsupported";
        case IR_CAPTURE_BUSY: return "rejected_busy";
        case IR_CAPTURE_MOVING: return "rejected_while_moving";
        case IR_CAPTURE_STOP_HELD: return "rejected_stop_held";
        case IR_CAPTURE_GAP: return "rejected_capture_gap";
        case IR_CAPTURE_INVALID_SAMPLE: return "rejected_invalid_sample";
        case IR_CAPTURE_CONTRAST: return "rejected_contrast";
        case IR_CAPTURE_NEED_BLACK: return "rejected_need_black";
        case IR_CAPTURE_CANCELLED_STOP: return "cancelled_on_stop";
        default: return "waiting";
    }
}
