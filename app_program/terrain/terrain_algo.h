#ifndef TERRAIN_ALGO_H
#define TERRAIN_ALGO_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/robot_data.h"
#include "../common/robot_time_math.h"
#include "../motion/encoder_math.h"

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} Gy511RawAxes;

/*
 * terrain_algo.h - calibrated terrain filtering and hump estimation.
 * Persistent task-local values use bounded scaled integers/bit-fields.
 */

typedef enum {
    TERRAIN_STATE_FLAT = 0,
    TERRAIN_STATE_CLIMBING,
    TERRAIN_STATE_CRESTING,
    TERRAIN_STATE_DESCENDING,
    TERRAIN_STATE_IMPACT
} TerrainState;

/* Signed Q16 units of one raw accelerometer LSB. Intermediate range may be
   wider than one raw int16 value after bias removal; saturate if necessary. */
typedef struct { int32_t x, y, z; } TerrainFilterQ16;

/* Calibration sums accumulate exactly 100 signed int16 samples. Each axis
   ranges -3276800..3276700, requiring 23 signed bits. No ISR/task shares
   this memory, so compiler bit-field writes are safe in the Terrain task.
   GNU C packed layout is asserted and checked by the target sizeof probe. */
typedef struct __attribute__((packed)) {
    int32_t x : 23;
    int32_t y : 23;
    int32_t z : 23;
} TerrainCalibSum;

_Static_assert(sizeof(TerrainCalibSum) == 9u, "TerrainCalibSum must use 69 bits in 9 bytes");

/* This is private persistent state of the one Terrain task, not a cross-core
   snapshot. Public telemetry uses terrain_pack(), an atomic 32-bit word.
   0..100 calibration samples: 7 bits; 0..10 flat-run: 4 bits;
   state 0..4: 3 bits; height/calibrated/window-quality flags: 1 bit each.
   Last/max codes are 7 bits each. */
typedef struct {
    TerrainFilterQ16 filtered_accel_q16;
    TerrainCalibSum calib_sum;
    uint16_t rise_q;                   /* 0.1 mm, saturates at 65535 */
    int16_t pitch_cdeg;                /* -9000..9000 centidegrees */
    uint16_t last_hump_code : 7;
    uint16_t max_hump_code : 7;
    uint8_t hump_count;
    uint8_t flat_run : 4;
    uint8_t calib_count : 7;
    uint8_t state : 3;
    uint8_t height_valid : 1;
    uint8_t is_calibrated : 1;
    uint8_t window_qualified : 1;
} TerrainContext;

_Static_assert(sizeof(TerrainContext) <= 36u, "TerrainContext packing regressed");

void terrain_algo_init(TerrainContext *ctx);
bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw);
/* Local travel includes half counts from averaging the two wheel deltas. */
void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw,
    float delta_counts, bool travel_qualified);
void terrain_algo_invalidate_window(TerrainContext *ctx);
void terrain_algo_calibration_restart(TerrainContext *ctx);
bool terrain_acceleration_qualified(const Gy511RawAxes *raw);

/*
 *	terrain_algo.h
 *	Terrain word (writer: Terrain, one aligned word, single store). Layout is
 *	PROVISIONAL until #28/#53 sign off the S4 rows; scale is D06.
 *	  bits  0-6   last hump height result (7 bits)
 *	  bits  7-13  maximum hump height result (7 bits)
 *	  bits 14-21  hump count (8 bits, saturating)
 *	  bits 22-24  terrain state (3 bits)
 *	  bit  25     height valid (the last encounter produced a valid height)
 *	  bit  26     calibrated
 *	A result code equal to rd_invalid_code(HUMP_RESULT_BITS) means no valid
 *	height; the reported height is code * HUMP_MM_PER_COUNT millimetres.
 */

#define TERRAIN_STATE_BITS 3u

static inline uint32_t terrain_pack(uint32_t last_code, uint32_t max_code, uint32_t count, uint32_t state,
                                    bool height_valid, bool calibrated)
{
    uint32_t invalid = rd_invalid_code(HUMP_RESULT_BITS);
    uint32_t last = last_code > invalid ? invalid : last_code;
    uint32_t max = max_code > invalid ? invalid : max_code;
    return last | (max << 7) | ((count > 255u ? 255u : count) << 14) |
           ((state & rd_mask(TERRAIN_STATE_BITS)) << 22) | ((height_valid ? 1u : 0u) << 25) |
           ((calibrated ? 1u : 0u) << 26);
}

static inline uint32_t terrain_last_code(uint32_t w) { return rd_field_get(w, 0u, HUMP_RESULT_BITS); }
static inline uint32_t terrain_max_code(uint32_t w) { return rd_field_get(w, 7u, HUMP_RESULT_BITS); }
static inline uint32_t terrain_hump_count(uint32_t w) { return rd_field_get(w, 14u, 8u); }
static inline uint32_t terrain_state_code(uint32_t w) { return rd_field_get(w, 22u, TERRAIN_STATE_BITS); }
static inline bool terrain_height_valid(uint32_t w) { return rd_field_get(w, 25u, 1u) != 0u; }
static inline bool terrain_calibrated(uint32_t w) { return rd_field_get(w, 26u, 1u) != 0u; }

/* Millimetres for a result code; false for the invalid code. */
static inline bool terrain_code_to_mm(uint32_t code, uint32_t *mm)
{
    if (code >= rd_invalid_code(HUMP_RESULT_BITS)) return false;
    *mm = code * HUMP_MM_PER_COUNT;
    return true;
}

typedef struct
{
    uint32_t sampled_ms, steady_since_ms;
    int16_t speed_mmps;
    uint8_t primed : 1;
    uint8_t steady : 1;
} TerrainTravelQuality;

bool terrain_travel_qualified(TerrainTravelQuality *quality,
    uint32_t now, const int32_t delta[2], bool acquisition_qualified);

/* Pending acquisition retains travel; failure discards its entire window. */
static inline float terrain_travel_take(uint32_t origin[2],
    const uint32_t count[2], bool acquired, bool discard)
{
    float travel = acquired && !discard ? enc_mean_travel(count, origin) : 0.0f;
    if (acquired || discard)
    {
        origin[0] = count[0];
        origin[1] = count[1];
    }
    return travel;
}

typedef struct
{
    uint32_t terrain, sampled_ms, serviced_ms, sequence;
    uint8_t acquisition_valid : 1;
} TerrainSnapshot;

static inline bool terrain_evidence_fresh(const TerrainSnapshot *sample,
    uint32_t now, uint32_t age_limit)
{
    return sample != 0 && sample->acquisition_valid != 0u &&
        sample->sequence != 0u &&
        robot_time_elapsed(now, sample->sampled_ms) <= age_limit &&
        robot_time_elapsed(now, sample->serviced_ms) <= age_limit;
}

/* A short impact observation must survive until Motion services braking.
   Call only on a fresh IMU sample; missing data cannot clear a cause. */
static inline bool terrain_impact_pending(bool previous, bool detected,
    bool brake_acknowledged)
{
    return detected || (previous && !brake_acknowledged);
}

#endif /* TERRAIN_ALGO_H */
