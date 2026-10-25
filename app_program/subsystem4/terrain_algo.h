/*
 * terrain_algo.h - Sensor filtering, tilt kinematics, hump estimation and impact detection.
 */

#ifndef TERRAIN_ALGO_H
#define TERRAIN_ALGO_H

#include <stdbool.h>
#include <stdint.h>
#include "drv_gy511.h"
#include "../common/robot_config.h"

typedef enum {
    TERRAIN_STATE_FLAT = 0,
    TERRAIN_STATE_CLIMBING,
    TERRAIN_STATE_CRESTING,
    TERRAIN_STATE_DESCENDING,
    TERRAIN_STATE_IMPACT
} TerrainState;

typedef struct {
    float x;
    float y;
    float z;
} Vec3f;

typedef struct {
    Vec3f        accel_offset;          /* Calibrated zero bias */
    Vec3f        filtered_accel;        /* Low-pass filtered acceleration */
    float        pitch_deg;             /* Calibrated body pitch angle */
    float        roll_deg;              /* Calibrated body roll angle */
    TerrainState state;                 /* Current terrain state */
    
    /* Hump estimator state */
    bool         height_valid;          /* Separate validity flag */
    uint16_t     rise_q;                /* Working climb height, 0.1 mm units, saturating (16-bit fixed point) */
    uint8_t      last_hump_code;        /* 7-bit result of the most recent valid hump */
    uint8_t      max_hump_code;         /* Greatest valid result this run (FR5); 0 until a hump */
    uint8_t      hump_count;            /* Valid complete encounters, saturating at 255 */
    uint8_t      hump_events[HUMP_EVENT_CAPACITY]; /* Ring of 7-bit results, oldest overwritten */
    uint8_t      hump_event_head;
    uint8_t      flat_run;              /* consecutive near-zero pitch samples while cresting */
    
    /* Calibration helper */
    bool         is_calibrated;
    uint16_t     calib_count;
    Vec3f        calib_sum;
} TerrainContext;

void terrain_algo_init(TerrainContext *ctx);
bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw);
/* delta_counts: forward encoder travel since the previous call, taken by the
   caller from a private origin (modular subtraction); converted to mm here. */
void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw, uint32_t delta_counts);
/* Restart the stationary start calibration (the robot moved while sampling). */
void terrain_algo_calibration_restart(TerrainContext *ctx);
void terrain_algo_reset_run_peak(TerrainContext *ctx);

#endif /* TERRAIN_ALGO_H */