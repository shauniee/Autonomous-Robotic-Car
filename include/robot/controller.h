#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include "robot/types.h"

void controller_init(VehicleController *controller);
void controller_step(VehicleController *controller, const SensorFrame *sensors,
                     RobotCommand command, uint32_t now_ms);
const char *controller_state_name(MissionState state);

#endif
