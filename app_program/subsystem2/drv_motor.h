#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H
#include "subsystem2.h"
/* Errors report software/BSP failures, not measured electrical output. */
ER drv_motor_init(void);
ER drv_motor_brake(void);
/* Signed duty per wheel, +forward / -backward, magnitude clamped to
   MOTION_MAX_DUTY. Non-finite input brakes and returns E_PAR. */
ER drv_motor_apply(float left, float right);
#endif
