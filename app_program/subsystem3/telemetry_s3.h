#ifndef TELEMETRY_S3_H
#define TELEMETRY_S3_H
#include "robot_control.h"
/* Copies diagnostics without waiting; success/failure never changes control. */
void telemetry_s3_update(const RobotControl *control, uint32_t now, bool test_inhibited);
#endif
