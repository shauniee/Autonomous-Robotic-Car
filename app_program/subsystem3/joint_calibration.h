#ifndef JOINT_CALIBRATION_H
#define JOINT_CALIBRATION_H
#include "../subsystem4/terrain_calibration.h"
/* Zero = wait, one = combined success, three = failure/cancellation. */
unsigned joint_calibration_result(bool black_done, bool cancelled,
                                  S4CalibrationStatus status, uint32_t now,
                                  uint32_t deadline);
#endif
