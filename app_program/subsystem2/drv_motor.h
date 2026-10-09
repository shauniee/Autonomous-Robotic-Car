#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H

#include <stdbool.h>

bool drv_motor_init(void);
bool drv_motor_brake(void);
bool drv_motor_apply(float left, float right);
/* Checked PWM register snapshot; neither speed nor measured stationarity. */
bool drv_motor_output_copy(float *left, float *right);

#endif
