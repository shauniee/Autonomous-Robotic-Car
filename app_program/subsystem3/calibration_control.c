#include "calibration_control.h"
#include <stddef.h>

void calibration_control_init(calibration_control_t *control)
{
    if (control != NULL)
    {
        *control = (calibration_control_t){0};
    }
}

unsigned calibration_control_feedback(IrCaptureKind before,
    const IrTracker *tracker)
{
    if ((tracker == NULL) || (before == IR_CAPTURE_NONE) ||
        (tracker->capture_kind != IR_CAPTURE_NONE))
    {
        return 0u;
    }
    switch (tracker->capture_result)
    {
        case IR_CAPTURE_BLACK_DONE: return 1u;
        case IR_CAPTURE_WHITE_DONE: return 2u;
        default: return 3u;
    }
}

unsigned calibration_control_inputs(calibration_control_t *control,
    IrTracker *tracker, bool capture_down, bool cancel_down, uint32_t now)
{
    if ((control == NULL) || (tracker == NULL))
    {
        return 0u;
    }
    unsigned chirps = 0u;
    if (cancel_down && (tracker->capture_kind != IR_CAPTURE_NONE))
    {
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_CANCELLED);
        chirps = 3u;
    }
    if (cancel_down && capture_down)
    {
        control->armed = false; /* Never replay a press begun under cancel. */
    }
    if (capture_down != control->candidate_down)
    {
        control->candidate_down = capture_down;
        control->changed_ms = now;
    }
    if ((uint32_t)(now - control->changed_ms) < CALIBRATION_DEBOUNCE_MS)
    {
        return chirps;
    }
    if (!control->candidate_down)
    {
        control->armed = true;
    }
    if (control->stable_down == control->candidate_down)
    {
        return chirps;
    }
    control->stable_down = control->candidate_down;
    if (!control->stable_down)
    {
        return chirps;
    }
    if (cancel_down)
    {
        tracker->capture_result = IR_CAPTURE_CANCEL_HELD;
        return 3u;
    }
    if (!control->armed)
    {
        return chirps;
    }
    control->armed = false;
    IrCaptureKind kind = tracker->calibration.black_set &&
        !tracker->calibration.white_set ? IR_CAPTURE_WHITE : IR_CAPTURE_BLACK;
    return ir_tracking_capture(tracker, kind, now) ? chirps : 3u;
}
