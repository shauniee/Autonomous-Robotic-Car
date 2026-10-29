#ifndef TERRAIN_EVIDENCE_H
#define TERRAIN_EVIDENCE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_time_math.h"

typedef struct
{
    uint32_t terrain, sampled_ms, serviced_ms, sequence;
    uint8_t acquisition_valid;
} S4Snapshot;

static inline bool terrain_evidence_fresh(const S4Snapshot *sample,
    uint32_t now, uint32_t age_limit)
{
    return sample != 0 && sample->acquisition_valid != 0u &&
        sample->sequence != 0u &&
        robot_time_elapsed(now, sample->sampled_ms) <= age_limit &&
        robot_time_elapsed(now, sample->serviced_ms) <= age_limit;
}

#endif /* TERRAIN_EVIDENCE_H */
