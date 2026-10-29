/* Qualified profile from ordered/unsorted stationary sonar rays. */
#ifndef PROFILE_EDGES_H
#define PROFILE_EDGES_H

#include "obstacle_profile.h"

typedef struct
{
    uint16_t depth_enter_mm;
    uint16_t depth_exit_mm;
    uint16_t background_separation_mm;
    uint16_t range_uncertainty_mm; /* UINT16_MAX = unknown */
    uint16_t planning_width_mm;
    uint8_t adjacent_rays;
    uint8_t maximum_gap_deg;
} s5_profile_config_t;

typedef struct
{
    uint16_t width_model_uncertainty_mm;
    uint16_t planning_width_mm;
    uint16_t foreground_rays : 4;
    uint16_t free_left_rays : 4;
    uint16_t free_right_rays : 4;
    uint16_t left_edge_valid : 1;
    uint16_t right_edge_valid : 1;
} s5_profile_evidence_t;

/* Requires a valid centre ray and adjacent foreground evidence, bracketed
   on both sides by valid background rays. Invalid/no-echo never means free.
   Optional background is a known empty-scene reference with matching angles.
   Configured uncertainty is a model input, never a calibrated sensor rating.
   False leaves both outputs unchanged. No waits or dynamic allocation. */
bool s5_profile_estimate(const ObstacleRay *rays, unsigned count,
    const ObstacleRay *background, const s5_profile_config_t *config,
    ObstacleProfile *out, s5_profile_evidence_t *evidence);

/* Up to two bounded refinements per supported object/background edge. */
unsigned s5_profile_fine_angles(const ObstacleRay *coarse, unsigned count,
    const s5_profile_config_t *config, int16_t *out, unsigned capacity);

#endif /* PROFILE_EDGES_H */
