/*
 * drv_gy511.h - Low-level I2C0 driver for LSM303DLHC on Grove 3 (GP4/GP5).
 * One bounded transaction at a time; no compliance certification implied.
 */

#ifndef DRV_GY511_H
#define DRV_GY511_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define GY511_I2C_ADDR_ACCEL 0x19u /* 7-bit accelerometer address */


typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} Gy511RawAxes;

/* Hardware lifecycle */
bool drv_gy511_init(void);
typedef enum { GY511_SAMPLE_ERROR, GY511_SAMPLE_PENDING, GY511_SAMPLE_FRESH } gy511_sample_result_t;
gy511_sample_result_t drv_gy511_sample(Gy511RawAxes *out_raw);

#endif /* DRV_GY511_H */
