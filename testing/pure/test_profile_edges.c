#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../app_program/obstacle/obstacle.h"

#define DEG_TO_RAD 0.0174532925f
#define RAYS 9u

/* Synthetic fixtures define a flat target face and an empty background plane.
   Their dimensions are test inputs, never claims about the mounted robot. */
static void scene(ObstacleRay rays[RAYS], unsigned width_mm)
{
    const int angles[] = {-20, -15, -10, -5, 0, 5, 10, 15, 20};
    for (unsigned i = 0u; i < RAYS; ++i)
    {
        float angle = angles[i] * DEG_TO_RAD;
        float lateral = 400.0f * tanf(angle);
        float plane = fabsf(lateral) <= (float)width_mm * 0.5f ? 400.0f : 1000.0f;
        rays[i] = obstacle_ray_make((int16_t)angles[i], (uint16_t)lroundf(plane / cosf(angle)), true);
    }
}

/* The whole sweep, as the robot scans it from where it stops: coarse rays, then
   the fine rays they ask for. */
static float sweep_ray_mm(int angle, float range, unsigned width, float left_wall)
{
    float t = angle * DEG_TO_RAD;
    float plane = fabsf(range * tanf(t)) <= width * 0.5f ? range : (angle > 0 ? left_wall : 2000.0f);
    return plane / cosf(t);
}

static void sweep(const obstacle_profile_config_t *config, float range, unsigned width,
    float left_wall, ObstacleProfile *profile)
{
    ObstacleRay rays[OBSTACLE_SCAN_CAPACITY];
    int8_t fine[OBSTACLE_SCAN_CAPACITY];
    obstacle_profile_evidence_t evidence = {0};
    unsigned count = 0u;
    for (int angle = -SERVO_MAX_DEG; angle <= SERVO_MAX_DEG; angle += OBSTACLE_COARSE_STEP_DEG)
    {
        rays[count++] = obstacle_ray_make((int16_t)angle,
            (uint16_t)lroundf(sweep_ray_mm(angle, range, width, left_wall)), true);
    }
    unsigned extra = obstacle_profile_fine_angles(rays, count, config, fine,
        OBSTACLE_SCAN_CAPACITY - count);
    for (unsigned i = 0u; i < extra; ++i)
    {
        rays[count++] = obstacle_ray_make(fine[i],
            (uint16_t)lroundf(sweep_ray_mm(fine[i], range, width, left_wall)), true);
    }
    assert(obstacle_profile_estimate(rays, count, NULL, config, profile, &evidence));
}

/* Rays captured on the car, 2026-11-05 (testing/integration/logs/scan.log). */
static void estimate_scan(const obstacle_profile_config_t *config, const int scan[][2],
    unsigned count, ObstacleProfile *profile)
{
    ObstacleRay rays[OBSTACLE_SCAN_CAPACITY];
    obstacle_profile_evidence_t evidence = {0};
    for (unsigned i = 0u; i < count; ++i)
    {
        rays[i] = obstacle_ray_make((int16_t)scan[i][0], (uint16_t)scan[i][1], scan[i][1] != 0);
    }
    assert(obstacle_profile_estimate(rays, count, NULL, config, profile, &evidence));
}

static void test_ghost_echoes(const obstacle_profile_config_t *config)
{
    ObstacleProfile profile = {0};
    /* Clear surroundings: 389 mm just past each edge is the box's ghost, so the
       room comes from the 850 mm and farther echoes behind it. */
    const int clear[][2] = {{-60, 1090}, {-50, 974}, {-40, 389}, {-35, 206}, {-30, 186},
        {-20, 177}, {-10, 190}, {0, 190}, {10, 177}, {20, 186}, {25, 196}, {30, 389},
        {40, 852}, {50, 866}, {60, 0}};
    estimate_scan(config, clear, sizeof clear / sizeof clear[0], &profile);
    assert(profile.left_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM);
    assert(profile.right_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM);
    /* The box sits right of centre, so its left edge is the nearer one. */
    assert(profile.obstacle_left_mm < 0);

    /* Ghosts with nothing real behind them prove no room. */
    const int cluttered[][2] = {{-60, 498}, {-50, 426}, {-40, 414}, {-35, 410}, {-30, 193},
        {-20, 183}, {-10, 173}, {0, 174}, {10, 183}, {20, 202}, {30, 221}, {35, 405},
        {40, 482}, {50, 484}, {60, 0}};
    estimate_scan(config, cluttered, sizeof cluttered / sizeof cluttered[0], &profile);
    assert(profile.left_clearance_mm < MISSION_BYPASS_MIN_CLEAR_MM);
    assert(profile.right_clearance_mm < MISSION_BYPASS_MIN_CLEAR_MM);
    puts("Obstacle: ghost echoes past the box do not take its room PASS");
}

/* Every supplied width gets enough clearance beside an open floor, and a wall
   beside the obstacle takes that side's clearance away. */
static void test_full_sweep_from_stop(const obstacle_profile_config_t *config)
{
    const float ranges[] = {110.0f, 130.0f, MISSION_OBSTACLE_STOP_MM};
    ObstacleProfile profile = {0};
    for (unsigned r = 0u; r < 3u; ++r)
    {
        for (unsigned width = 100u; width <= 200u; width += 50u)
        {
            sweep(config, ranges[r], width, 2000.0f, &profile);
            assert(profile.left_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM);
            assert(profile.right_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM);
        }
    }
    /* The wall 2000 mm away is past the profile limit: saturate, never wrap. */
    assert(profile.left_clearance_mm == OBSTACLE_PROFILE_MAX_MM);
    assert(profile.right_clearance_mm == OBSTACLE_PROFILE_MAX_MM);
    sweep(config, 130.0f, 200u, 300.0f, &profile);
    assert(profile.left_clearance_mm < MISSION_BYPASS_MIN_CLEAR_MM);
    assert(profile.right_clearance_mm >= MISSION_BYPASS_MIN_CLEAR_MM);
}

int main(void)
{
    const obstacle_profile_config_t config = {.depth_enter_mm = 30u, .depth_exit_mm = 60u,
        .background_separation_mm = 50u, .range_uncertainty_mm = 2u,
        .planning_width_mm = 200u, .adjacent_rays = 2u, .maximum_gap_deg = OBSTACLE_COARSE_STEP_DEG};
    test_full_sweep_from_stop(&config);
    test_ghost_echoes(&config);
    ObstacleRay rays[RAYS];
    ObstacleProfile profile = {0};
    obstacle_profile_evidence_t evidence = {0};
    for (unsigned width = 100u; width <= 200u; width += 50u)
    {
        scene(rays, width);
        assert(obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
        assert(profile.valid && profile.width_mm > 0u);
        assert(abs((int)profile.width_mm - (int)width) <= evidence.width_model_uncertainty_mm);
        assert(evidence.foreground_rays >= 2u && evidence.left_edge_valid && evidence.right_edge_valid);
        assert(evidence.planning_width_mm >= 200u);
        assert(evidence.planning_width_mm >= profile.width_mm);
    }
    scene(rays, 150u);
    assert(obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    uint16_t ordered_width = profile.width_mm;
    for (unsigned i = 0u; i < RAYS / 2u; ++i)
    {
        ObstacleRay temporary = rays[i];
        rays[i] = rays[RAYS - 1u - i];
        rays[RAYS - 1u - i] = temporary;
    }
    assert(obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    assert(profile.width_mm == ordered_width);
    scene(rays, 150u);
    obstacle_profile_config_t unknown = config;
    unknown.range_uncertainty_mm = UINT16_MAX;
    assert(obstacle_profile_estimate(rays, RAYS, NULL, &unknown, &profile, &evidence));
    assert(evidence.width_model_uncertainty_mm == OBSTACLE_UNCERTAINTY_UNKNOWN);
    assert(profile.left_clearance_mm == 0u && profile.right_clearance_mm == 0u);
    /* No echo past the box marks an edge but proves no clearance; with the box still
       on its far side it does not cut the box short. The centre ray must have an echo. */
    scene(rays, 150u);
    rays[0].valid = rays[1].valid = false;
    assert(obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    assert(profile.width_mm == ordered_width && profile.right_clearance_mm == 0u);
    assert(profile.left_clearance_mm > 0u);
    scene(rays, 150u);
    rays[3].valid = false;
    assert(obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    assert(profile.width_mm == ordered_width);
    scene(rays, 150u);
    rays[4].valid = false;
    assert(!obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    /* Rays logged in S5-U11 from scans that used to give no profile. */
    {
        static const int16_t logged[][OBSTACLE_SCAN_CAPACITY * 2u] = {
            {-50, 377, -40, 566, -30, 196, -20, 186, -10, 177, 0, 177, 10, 174, 20, 193, 30, 224, 40, 0, 50, 0, -35, 205},
            {-50, 363, -40, 298, -30, 196, -20, 185, -10, 177, 0, 177, 10, 173, 20, 205, 30, 381, 40, 384, 50, 361},
            {-50, 800, -40, 835, -30, 137, -20, 144, -10, 121, 0, 121, 10, 113, 20, 127, 30, 123, 40, 170, 50, 772, -35, 0, 45, 679}};
        static const unsigned logged_rays[] = {12u, 11u, 13u};
        for (unsigned scan = 0u; scan < 3u; ++scan)
        {
            ObstacleRay real[OBSTACLE_SCAN_CAPACITY];
            for (unsigned i = 0u; i < logged_rays[scan]; ++i)
            {
                real[i] = obstacle_ray_make(logged[scan][i * 2u], (uint16_t)logged[scan][i * 2u + 1u],
                    logged[scan][i * 2u + 1u] != 0);
            }
            obstacle_profile_config_t robot = config;
            robot.depth_exit_mm = OBSTACLE_PROFILE_DEPTH_EXIT_MM;
            assert(obstacle_profile_estimate(real, logged_rays[scan], NULL, &robot, &profile, &evidence));
        }
    }
    /* A no-echo ray ends the box for fine sampling, and a fine ray never repeats a coarse angle. */
    {
        ObstacleRay lost[5] = {obstacle_ray_make(-30, 0u, false), obstacle_ray_make(-20, 110u, true),
            obstacle_ray_make(0, 100u, true), obstacle_ray_make(20, 110u, true), obstacle_ray_make(30, 2000u, true)};
        int8_t extra[4];
        unsigned added = obstacle_profile_fine_angles(lost, 5u, &config, extra, 4u);
        assert(added == 2u && extra[0] == -25 && extra[1] == 25);
    }
    /* A single low centre ray among background samples is not an obstacle region. */
    scene(rays, 1u);
    assert(!obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    scene(rays, 150u);
    rays[1].angle_deg = rays[0].angle_deg;
    assert(!obstacle_profile_estimate(rays, RAYS, NULL, &config, &profile, &evidence));
    /* Optional background must match angles and show actual separation. */
    scene(rays, 150u);
    ObstacleRay background[RAYS];
    for (unsigned i = 0u; i < RAYS; ++i)
    {
        background[i] = obstacle_ray_make(rays[i].angle_deg,
            (uint16_t)lroundf(1000.0f / cosf(rays[i].angle_deg * DEG_TO_RAD)), true);
    }
    assert(obstacle_profile_estimate(rays, RAYS, background, &config, &profile, &evidence));
    memcpy(background, rays, sizeof background);
    assert(!obstacle_profile_estimate(rays, RAYS, background, &config, &profile, &evidence));
    assert(!obstacle_profile_estimate(rays, 4u, NULL, &config, &profile, &evidence));
    unknown = config;
    unknown.adjacent_rays = 0u;
    assert(!obstacle_profile_estimate(rays, RAYS, NULL, &unknown, &profile, &evidence));
    /* Fine sampling follows wide-object edges instead of only its centre. */
    ObstacleRay coarse[5] = {obstacle_ray_make(-20, 2000u, true),
        obstacle_ray_make(-10, 406u, true), obstacle_ray_make(0, 400u, true),
        obstacle_ray_make(10, 406u, true), obstacle_ray_make(20, 2000u, true)};
    int8_t fine[4];
    unsigned count = obstacle_profile_fine_angles(coarse, 5u, &config, fine, 4u);
    assert(count == 2u);
    assert(fine[0] == -15 && fine[1] == 15);
    assert(obstacle_profile_fine_angles(coarse, 5u, &config, fine, 0u) == 0u);
    puts("profile_edges: PASS (synthetic widths, conservative planning, unknowns and outlier rejection)");
    return 0;
}
