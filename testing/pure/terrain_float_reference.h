/* Preserved pre-conversion S4 algorithm: TEST-ONLY float reference, no firmware inclusion.
   This reference is deliberately independent of new fixed-point state. */
#ifndef TERRAIN_FLOAT_REFERENCE_H
#define TERRAIN_FLOAT_REFERENCE_H
#include "../../app_program/terrain/terrain_algo.h"
typedef struct { float x, y, z; } FloatVec3;
typedef struct {
    FloatVec3 accel_offset;
    FloatVec3 filtered_accel;
    float pitch_deg;
    TerrainState state;
    bool height_valid;
    uint16_t rise_q;
    uint8_t last_hump_code, max_hump_code, hump_count, flat_run;
    bool is_calibrated;
    uint16_t calib_count;
    FloatVec3 calib_sum;
} FloatTerrainContext;
void terrain_float_ref_init(FloatTerrainContext *ctx);
bool terrain_float_ref_calibrate_sample(FloatTerrainContext *ctx, const Gy511RawAxes *raw);
void terrain_float_ref_process(FloatTerrainContext *ctx, const Gy511RawAxes *raw, uint32_t delta_counts);
void terrain_float_ref_calibration_restart(FloatTerrainContext *ctx);
#endif
