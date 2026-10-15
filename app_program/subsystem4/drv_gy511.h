/*
 * drv_gy511.h - Low-level I2C0 driver for LSM303DLHC on Grove 3 (GP4/GP5).
 * One bounded transaction at a time; no compliance certification implied.
 */

#ifndef DRV_GY511_H
#define DRV_GY511_H

#include <stdbool.h>
#include <stdint.h>

#define GY511_I2C_ADDR_ACCEL 0x19u /* 7-bit accelerometer address */
#define GY511_I2C_ADDR_MAG 0x1Eu   /* 7-bit magnetometer address */

#define GY511_TIMEOUT_US 5000u /* Whole-transaction polling bound */

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} Gy511RawAxes;

typedef enum
{
    GY511_ERROR,
    GY511_WAIT,
    GY511_NEW
} Gy511Result;
Gy511Result drv_gy511_poll_accel(Gy511RawAxes *out);
Gy511Result drv_gy511_poll_mag(Gy511RawAxes *out);

/* Hardware lifecycle */
bool drv_gy511_init(void);
bool drv_gy511_read_accel(Gy511RawAxes *out_raw);
bool drv_gy511_read_mag(Gy511RawAxes *out_raw);

#endif /* DRV_GY511_H */
