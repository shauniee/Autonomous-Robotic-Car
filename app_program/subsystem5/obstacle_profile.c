#include "obstacle_profile.h"
#include <math.h>

#define DEG_TO_RAD 0.0174532925f

static bool ray_ok(const ObstacleRay *r)
{
    return r->valid && r->range_mm > 0u && r->angle_deg >= -SERVO_MAX_DEG && r->angle_deg <= SERVO_MAX_DEG;
}

static float forward_of(const ObstacleRay *r)
{
    return S5_SONAR_FORWARD_MM + (float)r->range_mm * cosf((float)r->angle_deg * DEG_TO_RAD);
}

static float left_of(const ObstacleRay *r)
{
    return S5_SONAR_LEFT_MM + (float)r->range_mm * sinf((float)r->angle_deg * DEG_TO_RAD);
}

static uint16_t clamp_u16(float v)
{
    return v <= 0.0f ? 0u : (v >= 65535.0f ? 65535u : (uint16_t)(v + 0.5f));
}

static int16_t clamp_i16(float v)
{
    return v <= -32768.0f ? -32768 : (v >= 32767.0f ? 32767 : (int16_t)(v < 0 ? v - 0.5f : v + 0.5f));
}

bool s5_profile_build(const ObstacleRay *rays, unsigned count, ObstacleProfile *out)
{
    if (rays == 0 || out == 0 || count < 5u || count > S5_SCAN_CAPACITY)
    {
        return false;
    }
    float closest = INFINITY;
    unsigned nearest = 0u, outer_left = 0u, outer_right = 0u;
    for (unsigned i = 0u; i < count; ++i)
    {
        if (rays[i].angle_deg > rays[outer_left].angle_deg) outer_left = i;
        if (rays[i].angle_deg < rays[outer_right].angle_deg) outer_right = i;
        if (!ray_ok(&rays[i]))
        {
            continue; /* no echo: unknown, never clear space */
        }
        float radius = hypotf(forward_of(&rays[i]), left_of(&rays[i]));
        if (radius < closest)
        {
            closest = radius;
            nearest = i;
        }
    }
    if (isinf(closest))
    {
        return false; /* no echo at all: nothing was profiled */
    }
    float front = forward_of(&rays[nearest]), left = left_of(&rays[nearest]);
    float xmin = left, xmax = left, free_left = left, free_right = left;
    for (unsigned i = 0u; i < count; ++i)
    {
        if (!ray_ok(&rays[i]))
        {
            continue;
        }
        float x = left_of(&rays[i]), y = forward_of(&rays[i]);
        if (y <= front + S5_BODY_LENGTH_MM + S5_CLEARANCE_MM)
        {
            xmin = fminf(xmin, x);
            xmax = fmaxf(xmax, x);
        }
        free_left = fmaxf(free_left, x);
        free_right = fminf(free_right, x);
    }
    /* A side is measured only if its outermost ray echoed; an unknown side
       reports zero clearance so Mission never bypasses into it. */
    *out = (ObstacleProfile){
        .closest_angle_deg = rays[nearest].angle_deg,
        .closest_mm = clamp_u16(closest),
        .width_mm = clamp_u16(xmax - xmin),
        .left_clearance_mm = ray_ok(&rays[outer_left]) ? clamp_u16(free_left - xmax) : 0u,
        .right_clearance_mm = ray_ok(&rays[outer_right]) ? clamp_u16(xmin - free_right) : 0u,
        .obstacle_forward_mm = clamp_i16(front),
        .obstacle_left_mm = clamp_i16(left),
        .valid = true};
    return true;
}

unsigned s5_fine_angles(const ObstacleRay *coarse, unsigned count, int16_t *out, unsigned max)
{
    if (coarse == 0 || out == 0 || count == 0u)
    {
        return 0u;
    }
    int nearest = -1;
    for (unsigned i = 0u; i < count; ++i)
    {
        if (coarse[i].valid && coarse[i].range_mm > 0u &&
            (nearest < 0 || coarse[i].range_mm < coarse[nearest].range_mm))
        {
            nearest = (int)i;
        }
    }
    if (nearest < 0)
    {
        return 0u;
    }
    unsigned n = 0u;
    const int steps[] = {-S5_FINE_STEP_DEG, S5_FINE_STEP_DEG, -2 * S5_FINE_STEP_DEG, 2 * S5_FINE_STEP_DEG};
    for (unsigned s = 0u; s < sizeof steps / sizeof steps[0] && n < max; ++s)
    {
        int angle = coarse[nearest].angle_deg + steps[s];
        bool taken = false;
        for (unsigned i = 0u; i < count; ++i)
        {
            taken = taken || coarse[i].angle_deg == angle;
        }
        if (!taken && angle >= -SERVO_MAX_DEG && angle <= SERVO_MAX_DEG)
        {
            out[n++] = (int16_t)angle;
        }
    }
    return n;
}

bool s5_scan_state_pack(s5_scan_state_t state, uint32_t *word)
{
    if ((unsigned)state >= S5_SCAN_STATE_COUNT || word == 0)
    {
        return false;
    }
    *word = (uint32_t)state;
    return true;
}

s5_scan_state_t s5_scan_state_unpack(uint32_t code)
{
    return code < S5_SCAN_STATE_COUNT ? (s5_scan_state_t)code : S5_SCAN_FAILED;
}

s5_scan_state_t s5_scan_guard(bool cancel, bool expired, bool stationary)
{
    return cancel ? S5_SCAN_CANCELLED : ((expired || !stationary) ? S5_SCAN_FAILED : S5_SCAN_COARSE);
}
