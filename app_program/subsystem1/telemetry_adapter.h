#ifndef S1_TELEMETRY_ADAPTER_H
#define S1_TELEMETRY_ADAPTER_H

#include "telemetry.h"
#include "../robot_control.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem3/ir_snapshot.h"

/* Application-only adapter boundary, also exercised by firmware fixtures.
   Neither this header nor the subsystem headers belong in lwIP sources. */
bool s1_telemetry_map(const S3IrSnapshot *ir,
    const RobotControlSnapshot *control, const MotionHealth *motion,
    uint32_t now, S1Telemetry *out);

#endif
