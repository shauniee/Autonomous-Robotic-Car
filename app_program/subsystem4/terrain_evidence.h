#ifndef TERRAIN_EVIDENCE_H
#define TERRAIN_EVIDENCE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_time_math.h"

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
