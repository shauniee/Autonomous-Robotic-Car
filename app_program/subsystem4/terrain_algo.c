/*
 * terrain_algo.c - Math and state machine implementation for Buddy 4.
 */

#include <math.h>
#include "terrain_algo.h"
#include "../common/robot_data.h"
#include "../subsystem2/encoder_math.h"
#include "../common/robot_config.h"

#define RAD_TO_DEG              57.2957795f
#define DEG_TO_RAD              0.0174532925f


/* Filter uses the existing LPF_ALPHA=0.20 as exact 1/5 in signed
   raw-LSB Q16 units. Keep this ratio in sync with D06 calibration if tuned. */
#define FILTER_DIV 5
#define RAW_Q_ONE 65536ll

static int64_t round_div(int64_t value, int64_t divisor)
{
    return value >= 0 ? (value + divisor / 2) / divisor
                      : -((-value + divisor / 2) / divisor);
}

static int32_t saturate_q16(int64_t value)
{
    if (value > INT32_MAX) return INT32_MAX;
    if (value < INT32_MIN) return INT32_MIN;
    return (int32_t)value;
}

static int32_t filter_axis(int32_t previous, int16_t raw, int32_t sum, bool vertical)
{
    /* Calibrated correction preserves fractional averages of all 100
       samples, rather than rounding the bias to a whole sensor count. */
    int64_t corrected = (int64_t)raw * RAW_Q_ONE -
                        round_div((int64_t)sum * RAW_Q_ONE, 100);
    if (vertical) corrected += (int64_t)(1.0f / ACCEL_MG_PER_LSB + 0.5f) * RAW_Q_ONE;
    return saturate_q16(round_div((int64_t)previous * (FILTER_DIV - 1) + corrected,
                                  FILTER_DIV));
}

void terrain_algo_init(TerrainContext *ctx)
{
    *ctx = (TerrainContext){0};
    ctx->filtered_accel_q16.z =
        (int32_t)((1.0f / ACCEL_MG_PER_LSB + 0.5f) * RAW_Q_ONE);
    ctx->state = TERRAIN_STATE_FLAT;
}

bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw)
{
    if (ctx->is_calibrated) return true;
    ctx->calib_sum.x += raw->x;
    ctx->calib_sum.y += raw->y;
    ctx->calib_sum.z += raw->z;
    ++ctx->calib_count;
    if (ctx->calib_count >= 100u)
    {
        ctx->is_calibrated = true;
        return true;
    }
    return false;
}

void terrain_algo_calibration_restart(TerrainContext *ctx)
{
    ctx->calib_count = 0;
    ctx->calib_sum = (TerrainCalibSum){0};
    ctx->is_calibrated = false;
}

/* Working height in 0.1 mm units, saturating at the 16-bit limit. */
static void rise_add(TerrainContext *ctx, float tenths_mm) {
    float next = (float)ctx->rise_q + tenths_mm;
    ctx->rise_q = next <= 0.0f ? 0u : (next >= 65535.0f ? 65535u : (uint16_t)(next + 0.5f));
}

void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw, uint32_t delta_counts) {
    float delta_dist_mm = (float)delta_counts / ENC_COUNTS_PER_MM;
    if (!ctx->is_calibrated) return;

    /* Local float values exist only inside this one processing release;
       persistent filter state and calibration history are scaled integers. */
    float cur_x = ((float)raw->x - (float)ctx->calib_sum.x / 100.0f) * ACCEL_MG_PER_LSB;
    float cur_y = ((float)raw->y - (float)ctx->calib_sum.y / 100.0f) * ACCEL_MG_PER_LSB;
    float cur_z = ((float)raw->z - (float)ctx->calib_sum.z / 100.0f) * ACCEL_MG_PER_LSB + 1.0f;

    /* Filter in high-resolution signed Q16 raw-LSB units. */
    ctx->filtered_accel_q16.x = filter_axis(ctx->filtered_accel_q16.x, raw->x, ctx->calib_sum.x, false);
    ctx->filtered_accel_q16.y = filter_axis(ctx->filtered_accel_q16.y, raw->y, ctx->calib_sum.y, false);
    ctx->filtered_accel_q16.z = filter_axis(ctx->filtered_accel_q16.z, raw->z, ctx->calib_sum.z, true);

    float filtered_x = ((float)ctx->filtered_accel_q16.x / (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float filtered_y = ((float)ctx->filtered_accel_q16.y / (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float filtered_z = ((float)ctx->filtered_accel_q16.z / (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float pitch_deg = atan2f(filtered_y,
                             sqrtf(filtered_x * filtered_x + filtered_z * filtered_z)) * RAD_TO_DEG;
    ctx->pitch_cdeg = (int16_t)lroundf(pitch_deg * 100.0f);

    /* 4. Impact detection (FR11): horizontal shock on level ground. IMPACT
       lasts while the shock does; Safety latches the stop it causes, and a
       hump in progress is abandoned. */
    float shock_horiz = sqrtf((cur_x * cur_x) + (cur_y * cur_y));
    if (shock_horiz > IMPACT_THRESHOLD_G && fabsf(pitch_deg) < 3.0f) {
        ctx->state = TERRAIN_STATE_IMPACT;
        return;
    }
    if (ctx->state == TERRAIN_STATE_IMPACT) {
        ctx->state = TERRAIN_STATE_FLAT;
    }

    /* 5. Hump progression state machine (Figure 7) */
    switch (ctx->state) {
        case TERRAIN_STATE_FLAT:
            if (pitch_deg > PITCH_CLIMB_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CLIMBING;
                ctx->rise_q = 0u;
                ctx->height_valid = false;
            }
            break;

        case TERRAIN_STATE_CLIMBING:
            if (delta_dist_mm > 0.0f) {
                rise_add(ctx, sinf(pitch_deg * DEG_TO_RAD) * delta_dist_mm * 10.0f);
            }
            if (pitch_deg < PITCH_CLIMB_THRESH_DEG && pitch_deg > PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_CRESTING;
            }
            break;

        case TERRAIN_STATE_CRESTING:
            if (pitch_deg < PITCH_DESC_THRESH_DEG) {
                ctx->state = TERRAIN_STATE_DESCENDING;
                ctx->flat_run = 0u;
            } else if (fabsf(pitch_deg) < 2.0f) {
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
            if (fabsf(pitch_deg) < 2.0f) {
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
