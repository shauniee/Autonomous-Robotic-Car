#ifndef TERRAIN_EVIDENCE_H
#define TERRAIN_EVIDENCE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_time_math.h"
#include "../subsystem2/encoder_math.h"

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
} S4Snapshot;

static inline bool terrain_evidence_fresh(const S4Snapshot *sample,
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

#endif /* TERRAIN_EVIDENCE_H */
