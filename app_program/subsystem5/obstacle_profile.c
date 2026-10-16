#include "subsystem5_internal.h"
#include "../common/robot_config.h"
#include <math.h>
bool s5_profile_build_config(ObstacleSnapshot *s, const RobotConfig *c)
{
    if (s == NULL || c == NULL || s->ray_count < 5u ||
        s->ray_count > S5_SCAN_CAPACITY)
    {
        return false;
    }
    float closest = INFINITY;
    unsigned nearest = 0u;
    for (unsigned i = 0u; i < s->ray_count; ++i)
    {
        if (!s->rays[i].valid || !isfinite(s->rays[i].range_m) ||
            s->rays[i].range_m <= 0.0f || !isfinite(s->rays[i].angle_deg) ||
            fabsf(s->rays[i].angle_deg) > 60.0f)
        {
            return false;
        }
        float theta = s->rays[i].angle_deg * 0.0174532925f;
        float forward = c->sonar_forward_m + s->rays[i].range_m * cosf(theta);
        float left = c->sonar_left_m + s->rays[i].range_m * sinf(theta);
        float radius = hypotf(forward, left);
        if (radius < closest)
        {
            nearest = i;
            closest = radius;
        }
    }
    float theta = s->rays[nearest].angle_deg * 0.0174532925f;
    float front = c->sonar_forward_m + s->rays[nearest].range_m * cosf(theta);
    float left = c->sonar_left_m + s->rays[nearest].range_m * sinf(theta);
    float xmin = left, xmax = left, free_left = left, free_right = left;
    for (unsigned i = 0u; i < s->ray_count; ++i)
    {
        float angle = s->rays[i].angle_deg * 0.0174532925f;
        float x = c->sonar_left_m + s->rays[i].range_m * sinf(angle);
        float y = c->sonar_forward_m + s->rays[i].range_m * cosf(angle);
        if (y <= front + c->body_length_m + c->clearance_m)
        {
            xmin = fminf(xmin, x);
            xmax = fmaxf(xmax, x);
        }
        free_left = fmaxf(free_left, x);
        free_right = fminf(free_right, x);
    }
    s->closest_m = closest;
    s->obstacle_forward_m = front;
    s->obstacle_left_m = left;
    s->width_m = xmax - xmin;
    s->left_clearance_m = fmaxf(0.0f, free_left - xmax);
    s->right_clearance_m = fmaxf(0.0f, xmin - free_right);
    return true;
}

bool s5_profile_build(ObstacleSnapshot *s)
{
    return s5_profile_build_config(s, &robot_config);
}
ScanStatus s5_scan_guard(bool cancel, bool expired, bool stationary)
{
    return cancel                   ? SCAN_CANCELLED
           : expired || !stationary ? SCAN_FAILED
                                    : SCAN_RUNNING;
}
