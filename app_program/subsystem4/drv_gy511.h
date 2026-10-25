/*
 * drv_gy511.h - Low-level I2C0 driver for LSM303DLHC on Grove 3 (GP4/GP5).
 * Complies with Barr-C standards and NFR12 (5 ms timeout).
 */

#ifndef DRV_GY511_H
#define DRV_GY511_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define GY511_I2C_ADDR_ACCEL    0x19u   /* 7-bit accelerometer address */
#define GY511_I2C_ADDR_MAG      0x1Eu   /* 7-bit magnetometer address */


typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} Gy511RawAxes;

/* Hardware lifecycle */
bool drv_gy511_init(void);
bool drv_gy511_read_accel(Gy511RawAxes *out_raw);
bool drv_gy511_read_mag(Gy511RawAxes *out_raw);

#endif /* DRV_GY511_H */
