/*
 * terrain_algo.c - Math and state machine implementation for Buddy 4.
 */

#include <math.h>
#include "terrain.h"
#include "../common/robot_data.h"
#include "../motion/motion.h"
#include "../common/robot_config.h"

#define RAD_TO_DEG              57.2957795f
#define DEG_TO_RAD              0.0174532925f


/* Filter weight is exactly 1/FILTER_DIV, in signed raw-LSB Q16 units. */
#define RAW_Q_ONE 65536ll
/* Stopped roll average in 1/256 centidegree; 1/16 per 20 ms is about 0.3 s. */
#define ROLL_BASE_ONE 256
#define ROLL_BASE_DIV 16

static int64_t round_div(int64_t value, int64_t divisor)
{
    return value >= 0 ? (value + divisor / 2) / divisor
                      : -((-value + divisor / 2) / divisor);
}

static int32_t saturate_q16(int64_t value)
{
    if (value > INT32_MAX)
    {
        return INT32_MAX;
    }
    if (value < INT32_MIN)
    {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int32_t filter_axis(int32_t previous, int16_t raw, int32_t sum,
    bool vertical)
{
    /* Calibrated correction preserves fractional averages of all 100
       samples, rather than rounding the bias to a whole sensor count. */
    int64_t corrected = (int64_t)raw * RAW_Q_ONE -
                        round_div((int64_t)sum * RAW_Q_ONE, 100);
    if (vertical)
    {
        corrected +=
            (int64_t)(1.0f / ACCEL_MG_PER_LSB + 0.5f) * RAW_Q_ONE;
    }
    return saturate_q16(
        round_div((int64_t)previous * (FILTER_DIV - 1) + corrected,
                  FILTER_DIV));
}

void terrain_algo_init(TerrainContext_t *ctx)
{
    *ctx = (TerrainContext_t){0};
    ctx->filtered_accel_q16.z =
        (int32_t)((1.0f / ACCEL_MG_PER_LSB + 0.5f) * RAW_Q_ONE);
    ctx->state = TERRAIN_STATE_FLAT;
}

bool terrain_algo_calibrate_sample(TerrainContext_t *ctx,
    const Gy511RawAxes_t *raw)
{
    if (ctx->is_calibrated)
    {
        return true;
    }
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

void terrain_algo_calibration_restart(TerrainContext_t *ctx)
{
    ctx->calib_count = 0;
    ctx->calib_sum = (TerrainCalibSum_t){0};
    ctx->is_calibrated = false;
}

float terrain_height_mm(const TerrainContext_t *p_ctx)
{
    float height_mm = 0.0f;

    if (TERRAIN_STATE_FLAT != p_ctx->state)
    {
        float roll_deg = (float)p_ctx->roll_cdeg / 100.0f;
        float tilt_deg = fabsf(roll_deg - ((float)p_ctx->roll_base_q8 /
            (100.0f * ROLL_BASE_ONE)));

        height_mm = TRACK_WIDTH_MM * sinf(tilt_deg * DEG_TO_RAD) *
            HUMP_CALIB_FACTOR;
    }
    return height_mm;
}

float terrain_acceleration_mg(const Gy511RawAxes_t *raw)
{
    return sqrtf((float)raw->x * raw->x +
        (float)raw->y * raw->y + (float)raw->z * raw->z) *
        ACCEL_MG_PER_LSB * 1000.0f;
}

void terrain_algo_process(TerrainContext_t *ctx, const Gy511RawAxes_t *raw,
    float delta_counts, bool stationary)
{
    if (!ctx->is_calibrated)
    {
        return;
    }

    /* Local float values exist only inside this one processing release;
       persistent filter state and calibration history are scaled integers. */
    ctx->filtered_accel_q16.x = filter_axis(ctx->filtered_accel_q16.x,
        raw->x, ctx->calib_sum.x, false);
    ctx->filtered_accel_q16.y = filter_axis(ctx->filtered_accel_q16.y,
        raw->y, ctx->calib_sum.y, false);
    ctx->filtered_accel_q16.z = filter_axis(ctx->filtered_accel_q16.z,
        raw->z, ctx->calib_sum.z, true);

    float filtered_x = ((float)ctx->filtered_accel_q16.x /
        (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float filtered_y = ((float)ctx->filtered_accel_q16.y /
        (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float filtered_z = ((float)ctx->filtered_accel_q16.z /
        (float)RAW_Q_ONE) * ACCEL_MG_PER_LSB;
    float pitch_deg = atan2f(filtered_y,
        sqrtf(filtered_x * filtered_x + filtered_z * filtered_z)) *
        RAD_TO_DEG;
    ctx->pitch_cdeg = (int16_t)lroundf(pitch_deg * 100.0f);
    float roll_deg = atan2f(filtered_x,
        sqrtf(filtered_y * filtered_y + filtered_z * filtered_z)) * RAD_TO_DEG;
    ctx->roll_cdeg = (int16_t)lroundf(roll_deg * 100.0f);

    /* 4. Lift or overturn (FR11): with the wheels on the floor the filtered
       vertical axis reads 1 g. Picking the car up, tipping it past about 45
       degrees or turning it over moves it further. Safety latches the stop,
       which ends the run; a hump in progress is abandoned. */
    if (fabsf(filtered_z - 1.0f) > LIFT_ACCEL_G)
    {
        ctx->state = TERRAIN_STATE_LIFTED;
        return;
    }
    if (TERRAIN_STATE_LIFTED == ctx->state)
    {
        ctx->state = TERRAIN_STATE_FLAT;
    }

    /* 5. Hump under one wheel: the course has no double humps, so the wheel
       lifts the chassis sideways, and pitch does not show a climb and descent
       (2026-11-02). Roll is judged against its average while stopped, as the
       boot pose offsets it; averaging while moving would follow a hump's slow
       ramp (2026-11-02). A short dip below the exit level does not end the
       encounter. */
    float tilt_deg = fabsf(roll_deg -
        (float)ctx->roll_base_q8 / (100.0f * ROLL_BASE_ONE));
    if (TERRAIN_STATE_FLAT == ctx->state)
    {
        /* Not zero mean travel: a pivot has that too, and tilts
           (2026-11-02). */
        if (stationary)
        {
            ctx->roll_base_q8 += ((int32_t)ctx->roll_cdeg * ROLL_BASE_ONE -
                ctx->roll_base_q8) / ROLL_BASE_DIV;
            return;
        }
        if (tilt_deg <= HUMP_ROLL_ENTER_DEG)
        {
            return;
        }
        ctx->state = TERRAIN_STATE_ON_HUMP;
        ctx->rise_q = 0u;
        ctx->travel_counts = 0u;
        ctx->flat_run = 0u;
        ctx->height_valid = false;
    }
    float rise_q = TRACK_WIDTH_MM * sinf(tilt_deg * DEG_TO_RAD) * 10.0f;
    if (rise_q > (float)ctx->rise_q)
    {
        float top = (float)rd_mask(TERRAIN_RISE_BITS);
        ctx->rise_q = rise_q >= top ? (uint16_t)top :
            (uint16_t)(rise_q + 0.5f);
    }
    if (tilt_deg >= HUMP_ROLL_EXIT_DEG)
    {
        if (delta_counts > 0.0f)
        {
            float travel = (float)ctx->travel_counts + delta_counts;
            float top = (float)rd_mask(TERRAIN_TRAVEL_BITS);
            ctx->travel_counts = travel >= top ? (uint16_t)top :
                (uint16_t)(travel + 0.5f);
        }
        ctx->flat_run = 0u;
        if ((float)ctx->travel_counts >=
            (float)HUMP_MAX_TRAVEL_MM * ENC_COUNTS_PER_MM)
        {
            /* Too long for a hump: this is the new ground, not an encounter. */
            ctx->roll_base_q8 = (int32_t)ctx->roll_cdeg * ROLL_BASE_ONE;
            ctx->state = TERRAIN_STATE_FLAT;
        }
    }
    else if (++ctx->flat_run >= HUMP_FLAT_SAMPLES)
    {
        /* Only a whole encounter of plausible height while driving adds an
           event; the top limit rejects the car being lifted by hand. */
        float mm = ((float)ctx->rise_q / 10.0f) * HUMP_CALIB_FACTOR;
        ctx->height_valid = mm >= (float)HUMP_MIN_HEIGHT_MM &&
            mm <= (float)HUMP_MAX_HEIGHT_MM &&
            (float)ctx->travel_counts >=
            (float)HUMP_MIN_TRAVEL_MM * ENC_COUNTS_PER_MM;
        if (ctx->height_valid)
        {
            uint8_t code = (uint8_t)rd_sat_valid(
                (uint32_t)(mm / (float)HUMP_MM_PER_COUNT + 0.5f),
                HUMP_RESULT_BITS);
            ctx->last_hump_code = code;
            if (code > ctx->max_hump_code)
            {
                ctx->max_hump_code = code;
            }
            if (ctx->hump_count < 255u)
            {
                ctx->hump_count++;
            }
        }
        ctx->state = TERRAIN_STATE_FLAT;
        ctx->flat_run = 0u;
    }
}

void terrain_algo_invalidate_window(TerrainContext_t *ctx)
{
    ctx->rise_q = 0u;
    ctx->flat_run = 0u;
    ctx->travel_counts = 0u;
    ctx->height_valid = false;
    if (ctx->state != TERRAIN_STATE_LIFTED)
    {
        ctx->state = TERRAIN_STATE_FLAT;
    }
}
