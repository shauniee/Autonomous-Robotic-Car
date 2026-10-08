#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H
void drv_motor_init(void);
void drv_motor_brake(void);
void drv_motor_apply(float left, float right);
#endif
