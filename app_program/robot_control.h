#ifndef ROBOT_CONTROL_H
#define ROBOT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

/* Reporting only; duties are requests, never measured wheel speed. */
typedef struct
{
    uint32_t observed_ms;
    uint32_t sequence;
    float left_duty;
    float right_duty;
    bool motion_enabled;
    char state[16];
    char reason[40];
} RobotControlSnapshot;

/* Task-context coherent copy; false leaves the destination unchanged. */
bool robot_control_copy(RobotControlSnapshot *snapshot);

#endif
