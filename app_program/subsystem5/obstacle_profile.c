#include "subsystem5.h"
#include "../common/robot_config.h"
#include <math.h>
bool s5_profile_build(ObstacleSnapshot *s)
{
    if (s == NULL || s->ray_count < 5u ||
        s->ray_count > S5_SCAN_CAPACITY) { return false; }
    float closest = INFINITY;
    unsigned nearest = 0u;
    for (unsigned i = 0u; i < s->ray_count; ++i)
    {
        if (!s->rays[i].valid || !isfinite(s->rays[i].range_m) ||
            s->rays[i].range_m <= 0.0f) { return false; }
        if (s->rays[i].range_m < closest)
        {
            nearest = i;
            closest = s->rays[i].range_m;
        }
    }
    float theta = s->rays[nearest].angle_deg * 0.0174532925f;
    float front = closest * cosf(theta);
    float left = closest * sinf(theta);
    float xmin = left, xmax = left, free_left = left, free_right = left;
    for (unsigned i = 0u; i < s->ray_count; ++i)
    {
        float angle = s->rays[i].angle_deg * 0.0174532925f;
        float x = s->rays[i].range_m * sinf(angle);
        float y = s->rays[i].range_m * cosf(angle);
        if (y <= front + robot_config.body_length_m + robot_config.clearance_m)
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
