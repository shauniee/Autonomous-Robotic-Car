#ifndef TERRAIN_ALGO_H
#define TERRAIN_ALGO_H
#include "drv_gy511.h"
#include "../common/robot_config.h"
typedef enum { TERRAIN_STATE_FLAT, TERRAIN_STATE_CLIMBING,
    TERRAIN_STATE_CRESTING, TERRAIN_STATE_DESCENDING,
    TERRAIN_STATE_IMPACT } TerrainState;
typedef enum { TERRAIN_STATIONARY, TERRAIN_ACCELERATING, TERRAIN_TURNING,
    TERRAIN_CLIMB, TERRAIN_DESCENT, TERRAIN_IMPACT } TerrainMovement;
typedef struct { float x, y, z; } Vec3f;
typedef struct
{
    Vec3f accel_offset, filtered_accel, calib_sum;
    float pitch_deg, roll_deg, current_hump_rise_m, roll_rise_m;
    float last_hump_height_m, max_hump_peak_m;
    uint32_t hump_started_ms;
    uint16_t calib_count;
    TerrainState state;
    bool height_valid, is_calibrated, window_valid;
} TerrainContext;
void terrain_algo_init(TerrainContext *ctx);
bool terrain_algo_calibrate_sample(TerrainContext *ctx,
    const Gy511RawAxes *raw);
void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw,
    float delta_dist_m, bool travel_valid, uint32_t now);
void terrain_algo_reset_run_peak(TerrainContext *ctx);
void terrain_algo_invalidate_window(TerrainContext *ctx);
#endif
