#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H
#include "subsystem2.h"
/* Errors report software/BSP failures, not measured electrical output. */
ER drv_motor_init(void);
ER drv_motor_brake(void);
ER drv_motor_apply(float left, float right);
#endif
