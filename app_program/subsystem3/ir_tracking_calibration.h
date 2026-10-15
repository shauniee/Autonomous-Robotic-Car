#ifndef IR_TRACKING_CALIBRATION_H
#define IR_TRACKING_CALIBRATION_H

#include "../common/button.h"
#include "ir_tracking.h"

typedef struct
{
    Button capture_button;
    bool joint_busy;
} calibration_control_t;

void calibration_control_init(calibration_control_t *control, uint32_t now);

unsigned calibration_control_feedback(IrCaptureKind before,
                                      const IrTracker *tracker);

unsigned calibration_handle_buttons(calibration_control_t *control,
                                    IrTracker *tracker, bool capture_down,
                                    bool cancel_down, uint32_t now);

#endif
