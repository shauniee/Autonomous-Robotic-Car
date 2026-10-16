#ifndef SUBSYSTEM5_INTERNAL_H
#define SUBSYSTEM5_INTERNAL_H

#include "subsystem5.h"
#include "../common/robot_config.h"

bool s5_profile_build(ObstacleSnapshot *snapshot);
bool s5_profile_build_config(ObstacleSnapshot *snapshot,
                             const RobotConfig *config);
ScanStatus s5_scan_guard(bool cancel, bool expired, bool stationary);

#endif
