#include "ir_tracking_calibration.h"
#include <stddef.h>

static unsigned cancel_capture_if_requested(IrTracker *tracker, bool cancel_down)
{
    if (!cancel_down || tracker->capture_kind == IR_CAPTURE_NONE)
    {
        return 0u;
    }

    ir_tracking_cancel_capture(tracker, IR_CAPTURE_CANCELLED);
    return 3u;
}

static ButtonEvent capture_press_received(
    calibration_control_t *control, 
    bool capture_down, 
    bool cancel_down, 
    uint32_t now)
{
    if (cancel_down && capture_down)
    {
        button_disarm(&control->capture_button);
    }

    return button_update(
        &control->capture_button, capture_down, now);
}

static IrCaptureKind next_capture_kind(const IrTracker *tracker)
{
    if (tracker->calibration.black_set && !tracker->calibration.white_set)
    {
        return IR_CAPTURE_WHITE;
    }

    return IR_CAPTURE_BLACK;
}

void calibration_control_init(
    calibration_control_t *control,
    uint32_t now)
{
    if (control != NULL)
    {
        button_init(&control->capture_button, now);
    }
}

unsigned calibration_control_feedback(IrCaptureKind before, const IrTracker *tracker)
{
    if ((tracker == NULL) || (before == IR_CAPTURE_NONE) || (tracker->capture_kind != IR_CAPTURE_NONE))
    {
        return 0u;
    }

    switch (tracker->capture_result)
    {
    case IR_CAPTURE_BLACK_DONE:
        return 1u;
    case IR_CAPTURE_WHITE_DONE:
        return 2u;
    default:
        return 3u;
    }
}

unsigned calibration_handle_buttons(
    calibration_control_t *control,
    IrTracker *tracker,
    bool capture_down,
    bool cancel_down,
    uint32_t now)
{
    if (control == NULL || tracker == NULL)
    {
        return 0u;
    }

    unsigned chirps = cancel_capture_if_requested(tracker, cancel_down);

    ButtonEvent event = capture_press_received(control, capture_down, cancel_down, now);

    bool press_transition = (event == BUTTON_EVENT_PRESSED) || (event == BUTTON_EVENT_IGNORED_PRESS);

    if (cancel_down && press_transition)
    {
        tracker->capture_result = IR_CAPTURE_CANCEL_HELD;
        return 3u;
    }

    if (event != BUTTON_EVENT_PRESSED)
    {
        return chirps;
    }

    IrCaptureKind kind = next_capture_kind(tracker);

    if (!ir_tracking_capture(tracker, kind, now))
    {
        return 3u;
    }

    return chirps;
}