#ifndef TERRAIN_H
#define TERRAIN_H

#include "../app.h"
#include "terrain_algo.h"
#include "../common/robot_config.h"

/*
 * terrain.h - Buddy 4 (Terrain) interface definitions.
 */

/* Subsystem lifecycle called by app_main.c */
ER terrain_init(void);

/* Terrain word (see terrain_algo.h): hump count and heights in one load. */
uint32_t terrain_read_word(void);
/* All fields belong to one acquisition/service publication, copied under
   a short cross-core mutex. Repeated reads never change sampled_ms. */

void terrain_step(uint32_t now_ms);
bool terrain_copy(TerrainSnapshot *out);
bool terrain_startup_complete(void);

/*
 * terrain.h - Low-level I2C0 driver for LSM303DLHC on Grove 3 (GP4/GP5).
 * One bounded transaction at a time; no compliance certification implied.
 */

#define GY511_I2C_ADDR_ACCEL 0x19u /* 7-bit accelerometer address */

/* Hardware lifecycle */
bool drv_gy511_init(void);
typedef enum { GY511_SAMPLE_ERROR, GY511_SAMPLE_PENDING, GY511_SAMPLE_FRESH } gy511_sample_result_t;
gy511_sample_result_t drv_gy511_sample(Gy511RawAxes *out_raw);

#endif /* TERRAIN_H */
