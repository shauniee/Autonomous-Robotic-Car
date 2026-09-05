#include "robot/controller.h"
#include "robot/subsystems.h"

int main(void) {
    VehicleController controller;
    controller_init(&controller);
    const MotionFeedback feedback = {0}; /* No simulated encoder model. */
    for (uint32_t tick = 0; tick < 8; ++tick) {
        const uint32_t now_ms = tick * 100;
        SensorFrame sensors = {
            .line = {.status = {true, now_ms}, .detected = true},
            .terrain = {.status = {true, now_ms}},
            .obstacle = {.status = {true, now_ms}, .front_distance_m = 1.0f}
        };
        RobotCommand command = COMMAND_NONE;
        if (tick == 1 || tick == 5) command = COMMAND_START;
        if (tick == 2) sensors.line.error = -0.5f;
        if (tick == 3) sensors.obstacle.front_distance_m = 0.1f;
        if (tick == 6) sensors.line.detected = false;
        if (tick == 7) command = COMMAND_STOP;
        controller_step(&controller, &sensors, command, now_ms);
        comms_publish(&controller, &sensors, &feedback, now_ms);
    }
    return 0;
}
