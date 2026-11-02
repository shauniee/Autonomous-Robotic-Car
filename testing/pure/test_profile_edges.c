#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../app_program/obstacle/obstacle.h"

#define DEG_TO_RAD 0.0174532925f

/* Synthetic fixtures define a flat target face and an empty background plane.
   Their dimensions are test inputs, never claims about the mounted robot. */
static void scene(ObstacleRay rays[OBSTACLE_SCAN_CAPACITY], unsigned width_mm)
{
    const int angles[] = {-20, -15, -10, -5, 0, 5, 10, 15, 20};
    for (unsigned i = 0u; i < OBSTACLE_SCAN_CAPACITY; ++i)
    {
        float angle = angles[i] * DEG_TO_RAD;
        float lateral = 400.0f * tanf(angle);
        float plane = fabsf(lateral) <= (float)width_mm * 0.5f ? 400.0f : 1000.0f;
        rays[i] = obstacle_ray_make((int16_t)angles[i], (uint16_t)lroundf(plane / cosf(angle)), true);
    }
}

int main(void)
{
    const obstacle_profile_config_t config = {.depth_enter_mm = 30u, .depth_exit_mm = 60u,
        .background_separation_mm = 50u, .range_uncertainty_mm = 2u,
        .planning_width_mm = 200u, .adjacent_rays = 2u, .maximum_gap_deg = OBSTACLE_COARSE_STEP_DEG};
    ObstacleRay rays[OBSTACLE_SCAN_CAPACITY];
    ObstacleProfile profile = {0};
    obstacle_profile_evidence_t evidence = {0};
    for (unsigned width = 100u; width <= 200u; width += 50u)
    {
        scene(rays, width);
        assert(obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
        assert(profile.valid && profile.width_mm > 0u);
        assert(abs((int)profile.width_mm - (int)width) <= evidence.width_model_uncertainty_mm);
        assert(evidence.foreground_rays >= 2u && evidence.left_edge_valid && evidence.right_edge_valid);
        assert(evidence.planning_width_mm >= 200u);
        assert(evidence.planning_width_mm >= profile.width_mm);
    }
    scene(rays, 150u);
    assert(obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    uint16_t ordered_width = profile.width_mm;
    for (unsigned i = 0u; i < OBSTACLE_SCAN_CAPACITY / 2u; ++i)
    {
        ObstacleRay temporary = rays[i];
        rays[i] = rays[OBSTACLE_SCAN_CAPACITY - 1u - i];
        rays[OBSTACLE_SCAN_CAPACITY - 1u - i] = temporary;
    }
    assert(obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    assert(profile.width_mm == ordered_width);
    scene(rays, 150u);
    obstacle_profile_config_t unknown = config;
    unknown.range_uncertainty_mm = UINT16_MAX;
    assert(obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &unknown, &profile, &evidence));
    assert(evidence.width_model_uncertainty_mm == UINT16_MAX);
    assert(profile.left_clearance_mm == 0u && profile.right_clearance_mm == 0u);
    /* No echo never qualifies an edge or fills a gap between object rays. */
    ObstacleProfile before = profile;
    rays[2].valid = false;
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    assert(memcmp(&before, &profile, sizeof before) == 0);
    scene(rays, 150u);
    rays[4].valid = false;
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    /* A single low centre ray among background samples is not an obstacle region. */
    scene(rays, 1u);
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    scene(rays, 150u);
    rays[1].angle_deg = rays[0].angle_deg;
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &config, &profile, &evidence));
    /* Optional background must match angles and show actual separation. */
    scene(rays, 150u);
    ObstacleRay background[OBSTACLE_SCAN_CAPACITY];
    for (unsigned i = 0u; i < OBSTACLE_SCAN_CAPACITY; ++i)
    {
        background[i] = obstacle_ray_make(rays[i].angle_deg,
            (uint16_t)lroundf(1000.0f / cosf(rays[i].angle_deg * DEG_TO_RAD)), true);
    }
    assert(obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, background, &config, &profile, &evidence));
    memcpy(background, rays, sizeof background);
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, background, &config, &profile, &evidence));
    assert(!obstacle_profile_estimate(rays, 4u, NULL, &config, &profile, &evidence));
    unknown = config;
    unknown.adjacent_rays = 0u;
    assert(!obstacle_profile_estimate(rays, OBSTACLE_SCAN_CAPACITY, NULL, &unknown, &profile, &evidence));
    /* Fine sampling follows wide-object edges instead of only its centre. */
    ObstacleRay coarse[5] = {obstacle_ray_make(-20, 2000u, true),
        obstacle_ray_make(-10, 406u, true), obstacle_ray_make(0, 400u, true),
        obstacle_ray_make(10, 406u, true), obstacle_ray_make(20, 2000u, true)};
    int16_t fine[4];
    unsigned count = obstacle_profile_fine_angles(coarse, 5u, &config, fine, 4u);
    assert(count == 2u);
    assert(fine[0] == -15 && fine[1] == 15);
    assert(obstacle_profile_fine_angles(coarse, 5u, &config, fine, 0u) == 0u);
    puts("profile_edges: PASS (synthetic widths, conservative planning, unknowns and outlier rejection)");
    return 0;
}
