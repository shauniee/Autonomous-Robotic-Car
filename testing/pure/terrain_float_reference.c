/* Legacy pre-change float reference copied for regression comparison only. */
/*
 * terrain_algo.c - Math and state machine implementation for Buddy 4.
 */

#include <math.h>
#include "terrain_float_reference.h"
#include "../../app_program/common/robot_data.h"
#include "../../app_program/subsystem2/encoder_math.h"
#include "../../app_program/common/robot_config.h"

#define RAD_TO_DEG              57.2957795f
#define DEG_TO_RAD              0.0174532925f


void terrain_float_ref_init(FloatTerrainContext *ctx) {
    ctx->accel_offset = (FloatVec3){0.0f, 0.0f, 0.0f};
    ctx->filtered_accel = (FloatVec3){0.0f, 0.0f, 1.0f};
    ctx->pitch_deg = 0.0f;
    ctx->state = TERRAIN_STATE_FLAT;
    ctx->height_valid = false;
    ctx->rise_q = 0u;
    ctx->last_hump_code = 0u;
    ctx->max_hump_code = 0u;
    ctx->hump_count = 0u;
    ctx->flat_run = 0u;
    ctx->is_calibrated = false;
    ctx->calib_count = 0;
    ctx->calib_sum = (FloatVec3){0.0f, 0.0f, 0.0f};
}

bool terrain_float_ref_calibrate_sample(FloatTerrainContext *ctx, const Gy511RawAxes *raw) {
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

void terrain_float_ref_calibration_restart(FloatTerrainContext *ctx) {
    ctx->calib_count = 0;
    ctx->calib_sum = (FloatVec3){0.0f, 0.0f, 0.0f};
}

/* Working height in 0.1 mm units, saturating at the 16-bit limit. */
static void rise_add(FloatTerrainContext *ctx, float tenths_mm) {
    float next = (float)ctx->rise_q + tenths_mm;
    ctx->rise_q = next <= 0.0f ? 0u : (next >= 65535.0f ? 65535u : (uint16_t)(next + 0.5f));
}

void terrain_float_ref_process(FloatTerrainContext *ctx, const Gy511RawAxes *raw, uint32_t delta_counts) {
    float delta_dist_mm = (float)delta_counts / ENC_COUNTS_PER_MM;
    if (!ctx->is_calibrated) return;

    /* 1. Convert to physical g with bias removal */
    FloatVec3 cur;
    cur.x = ((float)raw->x * ACCEL_MG_PER_LSB) - ctx->accel_offset.x;
    cur.y = ((float)raw->y * ACCEL_MG_PER_LSB) - ctx->accel_offset.y;
    cur.z = ((float)raw->z * ACCEL_MG_PER_LSB) - ctx->accel_offset.z;

    /* 2. Low-pass filter (IIR) to strip chassis vibration */
    ctx->filtered_accel.x = (ctx->filtered_accel.x * (1.0f - LPF_ALPHA)) + (cur.x * LPF_ALPHA);
    ctx->filtered_accel.y = (ctx->filtered_accel.y * (1.0f - LPF_ALPHA)) + (cur.y * LPF_ALPHA);
    ctx->filtered_accel.z = (ctx->filtered_accel.z * (1.0f - LPF_ALPHA)) + (cur.z * LPF_ALPHA);

    /* 3. Compute pitch angle */
    ctx->pitch_deg = atan2f(ctx->filtered_accel.y,
                            sqrtf((ctx->filtered_accel.x * ctx->filtered_accel.x) + 
                                  (ctx->filtered_accel.z * ctx->filtered_accel.z))) * RAD_TO_DEG;

    /* 4. Impact detection (FR11): horizontal shock on level ground. IMPACT
       lasts while the shock does; Safety latches the stop it causes, and a
       hump in progress is abandoned. */
    float shock_horiz = sqrtf((cur.x * cur.x) + (cur.y * cur.y));
    if (shock_horiz > IMPACT_THRESHOLD_G && fabsf(ctx->pitch_deg) < 3.0f) {
        ctx->state = TERRAIN_STATE_IMPACT;
        return;
    }
    if (ctx->state == TERRAIN_STATE_IMPACT) {
        ctx->state = TERRAIN_STATE_FLAT;
    }

    /* 5. Hump progression state machine (Figure 7) */
    switch (ctx->state) {
        case TERRAIN_STATE_FLAT:
            if (ctx->pitch_deg > PITCH_CLIMB_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CLIMBING;
                ctx->rise_q = 0u;
                ctx->height_valid = false;
            }
            break;

        case TERRAIN_STATE_CLIMBING:
            if (delta_dist_mm > 0.0f) {
                rise_add(ctx, sinf(ctx->pitch_deg * DEG_TO_RAD) * delta_dist_mm * 10.0f);
            }
            if (ctx->pitch_deg < PITCH_CLIMB_THRESH_DEG && ctx->pitch_deg > PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CRESTING;
            }
            break;

        case TERRAIN_STATE_CRESTING:
            if (ctx->pitch_deg < PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_DESCENDING;
                ctx->flat_run = 0u;
            } else if (fabsf(ctx->pitch_deg) < 2.0f) {
                /* The filtered pitch passes through zero between climb and descent;
                   only a sustained flat reading is a premature flatten. */
                if (++ctx->flat_run >= HUMP_FLAT_SAMPLES) {
                    ctx->state = TERRAIN_STATE_FLAT;
                    ctx->flat_run = 0u;
                }
            } else {
                ctx->flat_run = 0u;
            }
            break;

        case TERRAIN_STATE_DESCENDING:
            if (fabsf(ctx->pitch_deg) < 2.0f) {
                /* Complete climb-crest-descent window finished: only a valid, complete
                   encounter adds an event. */
                float mm = ((float)ctx->rise_q / 10.0f) * HUMP_CALIB_FACTOR;
                ctx->height_valid = mm >= (float)HUMP_MIN_HEIGHT_MM;
                if (ctx->height_valid) {
                    uint8_t code = (uint8_t)rd_sat_valid((uint32_t)(mm / (float)HUMP_MM_PER_COUNT + 0.5f), HUMP_RESULT_BITS);
                    ctx->last_hump_code = code;
                    if (code > ctx->max_hump_code) ctx->max_hump_code = code;
                    if (ctx->hump_count < 255u) ctx->hump_count++;
                }
                ctx->state = TERRAIN_STATE_FLAT;
            }
            break;

        case TERRAIN_STATE_IMPACT:
            break;
    }
}
