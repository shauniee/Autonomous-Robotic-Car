#include "joint_calibration.h"
#include "../common/robot_time.h"
unsigned joint_calibration_result(bool black_done, bool cancelled,
                                  S4CalibrationStatus status, uint32_t now,
                                  uint32_t deadline)
{
    if (cancelled || status == S4_CAL_FAILED || status == S4_CAL_CANCELLED ||
        robot_due(now, deadline))
    {
        return 3u;
    }
    return black_done && status == S4_CAL_DONE ? 1u : 0u;
}
