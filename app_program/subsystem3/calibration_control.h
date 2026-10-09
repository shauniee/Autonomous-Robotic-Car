#ifndef CALIBRATION_CONTROL_H
#define CALIBRATION_CONTROL_H

#include "ir_tracking.h"

#define CALIBRATION_DEBOUNCE_MS 30u

typedef struct
{
    bool candidate_down;
    bool stable_down;
    bool armed;
    uint32_t changed_ms;
} calibration_control_t;

void calibration_control_init(calibration_control_t *control);
unsigned calibration_control_inputs(calibration_control_t *control, IrTracker *tracker, bool capture_down, bool cancel_down, uint32_t now);
unsigned calibration_control_feedback(IrCaptureKind before, const IrTracker *tracker);

#endif
