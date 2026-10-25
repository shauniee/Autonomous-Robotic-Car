/*
 * terrain_algo.c - Math and state machine implementation for Buddy 4.
 */

#include <math.h>
#include "terrain_algo.h"
#include "../common/robot_config.h"

#define RAD_TO_DEG              57.2957795f
#define DEG_TO_RAD              0.0174532925f


void terrain_algo_init(TerrainContext *ctx) {
    ctx->accel_offset = (Vec3f){0.0f, 0.0f, 0.0f};
    ctx->filtered_accel = (Vec3f){0.0f, 0.0f, 1.0f};
    ctx->pitch_deg = 0.0f;
    ctx->roll_deg = 0.0f;
    ctx->state = TERRAIN_STATE_FLAT;
    ctx->height_valid = false;
    ctx->current_hump_rise_m = 0.0f;
    ctx->last_hump_height_m = 0.0f;
    ctx->max_hump_peak_m = 0.0f;
    ctx->is_calibrated = false;
    ctx->calib_count = 0;
    ctx->calib_sum = (Vec3f){0.0f, 0.0f, 0.0f};
}

bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw) {
    if (ctx->is_calibrated) return true;

    ctx->calib_sum.x += (float)raw->x * ACCEL_MG_PER_LSB;
    ctx->calib_sum.y += (float)raw->y * ACCEL_MG_PER_LSB;
    ctx->calib_sum.z += (float)raw->z * ACCEL_MG_PER_LSB;
    ctx->calib_count++;

    if (ctx->calib_count >= 100) {
        ctx->accel_offset.x = ctx->calib_sum.x / 100.0f;
        ctx->accel_offset.y = ctx->calib_sum.y / 100.0f;
        ctx->accel_offset.z = (ctx->calib_sum.z / 100.0f) - 1.0f; /* Normalize Z to 1g */
        ctx->is_calibrated = true;
        return true;
    }
    return false;
}

void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw, float delta_dist_m) {
    if (!ctx->is_calibrated) return;

    /* 1. Convert to physical g with bias removal */
    Vec3f cur;
    cur.x = ((float)raw->x * ACCEL_MG_PER_LSB) - ctx->accel_offset.x;
    cur.y = ((float)raw->y * ACCEL_MG_PER_LSB) - ctx->accel_offset.y;
    cur.z = ((float)raw->z * ACCEL_MG_PER_LSB) - ctx->accel_offset.z;

    /* 2. Low-pass filter (IIR) to strip chassis vibration */
    ctx->filtered_accel.x = (ctx->filtered_accel.x * (1.0f - LPF_ALPHA)) + (cur.x * LPF_ALPHA);
    ctx->filtered_accel.y = (ctx->filtered_accel.y * (1.0f - LPF_ALPHA)) + (cur.y * LPF_ALPHA);
    ctx->filtered_accel.z = (ctx->filtered_accel.z * (1.0f - LPF_ALPHA)) + (cur.z * LPF_ALPHA);

    /* 3. Compute pitch and roll angles */
    ctx->pitch_deg = atan2f(ctx->filtered_accel.y,
                            sqrtf((ctx->filtered_accel.x * ctx->filtered_accel.x) + 
                                  (ctx->filtered_accel.z * ctx->filtered_accel.z))) * RAD_TO_DEG;
    ctx->roll_deg  = atan2f(-ctx->filtered_accel.x, ctx->filtered_accel.z) * RAD_TO_DEG;

    /* 4. Impact detection (FR11): check horizontal deceleration */
    float shock_horiz = sqrtf((cur.x * cur.x) + (cur.y * cur.y));
    if (shock_horiz > IMPACT_THRESHOLD_G && fabsf(ctx->pitch_deg) < 3.0f) {
        ctx->state = TERRAIN_STATE_IMPACT;
        return;
    }

    /* 5. Hump progression state machine (Figure 7) */
    switch (ctx->state) {
        case TERRAIN_STATE_FLAT:
            if (ctx->pitch_deg > PITCH_CLIMB_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CLIMBING;
                ctx->current_hump_rise_m = 0.0f;
                ctx->height_valid = false;
            }
            break;

        case TERRAIN_STATE_CLIMBING:
            if (delta_dist_m > 0.0f) {
                ctx->current_hump_rise_m += sinf(ctx->pitch_deg * DEG_TO_RAD) * delta_dist_m;
            }
            if (ctx->pitch_deg < PITCH_CLIMB_THRESH_DEG && ctx->pitch_deg > PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CRESTING;
            }
            break;

        case TERRAIN_STATE_CRESTING:
            if (ctx->pitch_deg < PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_DESCENDING;
            } else if (fabsf(ctx->pitch_deg) < 2.0f) {
                /* Premature flatten without descent */
                ctx->state = TERRAIN_STATE_FLAT;
            }
            break;

        case TERRAIN_STATE_DESCENDING:
            if (fabsf(ctx->pitch_deg) < 2.0f) {
                /* Complete climb-crest-descent window finished! */
                ctx->last_hump_height_m = ctx->current_hump_rise_m * HUMP_CALIB_FACTOR;
                ctx->height_valid = (ctx->last_hump_height_m > 0.005f); /* >5mm minimum threshold */
                
                if (ctx->height_valid && (ctx->last_hump_height_m > ctx->max_hump_peak_m)) {
                    ctx->max_hump_peak_m = ctx->last_hump_height_m;
                }
                ctx->state = TERRAIN_STATE_FLAT;
            }
            break;

        case TERRAIN_STATE_IMPACT:
            /* Latch state until system reset */
            break;
    }
}

void terrain_algo_reset_run_peak(TerrainContext *ctx) {
    ctx->max_hump_peak_m = 0.0f;
}
