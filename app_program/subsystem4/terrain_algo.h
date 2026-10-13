/*
 * terrain_algo.h - Sensor filtering, tilt kinematics, hump estimation and impact detection.
 */

#ifndef TERRAIN_ALGO_H
#define TERRAIN_ALGO_H

#include <stdbool.h>
#include <stdint.h>
#include "drv_gy511.h"

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
    float        current_hump_rise_m;   /* Accumulated climb height */
    float        last_hump_height_m;    /* Height of most recent valid hump */
    float        max_hump_peak_m;       /* Greatest valid peak in current run (FR5) */
    
    /* Calibration helper */
    bool         is_calibrated;
    uint16_t     calib_count;
    Vec3f        calib_sum;
} TerrainContext;

void terrain_algo_init(TerrainContext *ctx);
bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw);
void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw, float delta_dist_m);
void terrain_algo_reset_run_peak(TerrainContext *ctx);

#endif /* TERRAIN_ALGO_H */