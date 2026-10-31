/*
 * terrain_algo.h - calibrated terrain filtering and hump estimation.
 * Persistent task-local values use bounded scaled integers/bit-fields.
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
   state 0..4: 3 bits; height/calibrated flags: 1 bit each.
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
} TerrainContext;

_Static_assert(sizeof(TerrainContext) <= 36u, "TerrainContext packing regressed");

void terrain_algo_init(TerrainContext *ctx);
bool terrain_algo_calibrate_sample(TerrainContext *ctx, const Gy511RawAxes *raw);
/* Local travel includes half counts from averaging the two wheel deltas. */
void terrain_algo_process(TerrainContext *ctx, const Gy511RawAxes *raw, float delta_counts);
void terrain_algo_invalidate_window(TerrainContext *ctx);
void terrain_algo_calibration_restart(TerrainContext *ctx);

#endif /* TERRAIN_ALGO_H */
