#include "terrain_algo.h"
#include <math.h>
#include <string.h>
static Vec3f mapped(const Gy511RawAxes *raw)
{
    float axes[3] = {(float)raw->x * 0.001f, (float)raw->y * 0.001f,
        (float)raw->z * 0.001f};
    return (Vec3f){axes[robot_config.lateral_axis] * robot_config.lateral_sign,
        axes[robot_config.forward_axis] * robot_config.forward_sign,
        axes[robot_config.vertical_axis] * robot_config.vertical_sign};
}
void terrain_algo_init(TerrainContext *c)
{
    memset(c, 0, sizeof(*c));
    c->filtered_accel.z = 1.0f;
}
bool terrain_algo_calibrate_sample(TerrainContext *c,
    const Gy511RawAxes *raw)
{
    Vec3f v = mapped(raw);
    float g = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (!isfinite(g) || fabsf(g - 1.0f) > 0.15f)
    {
        c->calib_count = 0u;
        c->calib_sum = (Vec3f){0};
        return false;
    }
    if (c->is_calibrated) { return true; }
    c->calib_sum.x += v.x;
    c->calib_sum.y += v.y;
    c->calib_sum.z += v.z;
    if (++c->calib_count >= 100u)
    {
        c->accel_offset = (Vec3f){c->calib_sum.x / 100.0f,
            c->calib_sum.y / 100.0f, c->calib_sum.z / 100.0f - 1.0f};
        c->is_calibrated = true;
    }
    return c->is_calibrated;
}
void terrain_algo_invalidate_window(TerrainContext *c)
{
    if (c->state != TERRAIN_STATE_IMPACT) { c->state = TERRAIN_STATE_FLAT; }
    c->window_valid = false;
    c->height_valid = false;
    c->current_hump_rise_m = c->roll_rise_m = 0.0f;
}
void terrain_algo_process(TerrainContext *c, const Gy511RawAxes *raw,
    float travel, bool travel_valid, uint32_t now)
{
    if (!c->is_calibrated) { return; }
    Vec3f v = mapped(raw);
    v.x -= c->accel_offset.x;
    v.y -= c->accel_offset.y;
    v.z -= c->accel_offset.z;
    c->filtered_accel.x += 0.2f * (v.x - c->filtered_accel.x);
    c->filtered_accel.y += 0.2f * (v.y - c->filtered_accel.y);
    c->filtered_accel.z += 0.2f * (v.z - c->filtered_accel.z);
    Vec3f f = c->filtered_accel;
    c->pitch_deg = atan2f(f.y, sqrtf(f.x * f.x + f.z * f.z)) * 57.2957795f;
    c->roll_deg = atan2f(-f.x, f.z) * 57.2957795f;
    if (robot_config_terrain_valid() &&
        hypotf(v.x, v.y) > robot_config.impact_g)
    {
        c->state = TERRAIN_STATE_IMPACT;
        c->height_valid = false;
        return;
    }
    if (c->state == TERRAIN_STATE_IMPACT) { return; }
    float g = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    bool usable = robot_config_terrain_valid() && travel_valid &&
        isfinite(travel) && travel >= 0.0f &&
        fabsf(g - 1.0f) <= robot_config.gravity_tolerance_g;
    if (!usable || (c->state != TERRAIN_STATE_FLAT &&
        (uint32_t)(now - c->hump_started_ms) >= robot_config.hump_ms))
    {
        terrain_algo_invalidate_window(c);
        return;
    }
    float pitch_limit = robot_config.hump_pitch_deg;
    bool raised = c->pitch_deg > pitch_limit ||
        fabsf(c->roll_deg) > robot_config.hump_roll_deg;
    bool baseline = fabsf(c->pitch_deg) < pitch_limit * 0.5f &&
        fabsf(c->roll_deg) < robot_config.hump_roll_deg * 0.5f;
    if (c->state == TERRAIN_STATE_FLAT && raised)
    {
        c->state = TERRAIN_STATE_CLIMBING;
        c->window_valid = true;
        c->height_valid = false;
        c->hump_started_ms = now;
        c->current_hump_rise_m = c->roll_rise_m = 0.0f;
    }
    if (c->state == TERRAIN_STATE_CLIMBING)
    {
        c->current_hump_rise_m += fmaxf(0.0f,
            sinf(c->pitch_deg * 0.0174532925f) * travel);
        c->roll_rise_m = fmaxf(c->roll_rise_m,
            robot_config.track_m * 0.5f *
            sinf(fabsf(c->roll_deg) * 0.0174532925f));
        if (!raised) { c->state = TERRAIN_STATE_CRESTING; }
    }
    else if (c->state == TERRAIN_STATE_CRESTING)
    {
        if (c->pitch_deg < -pitch_limit ||
            (c->roll_rise_m > 0.0f && baseline))
        {
            c->state = TERRAIN_STATE_DESCENDING;
        }
    }
    else if (c->state == TERRAIN_STATE_DESCENDING && baseline)
    {
        float height = robot_config.height_factor * c->current_hump_rise_m +
            robot_config.roll_height_factor * c->roll_rise_m;
        c->height_valid = c->window_valid && isfinite(height) && height > 0.0f;
        if (c->height_valid)
        {
            c->last_hump_height_m = height;
            c->max_hump_peak_m = fmaxf(c->max_hump_peak_m, height);
        }
        c->state = TERRAIN_STATE_FLAT;
    }
}
void terrain_algo_reset_run_peak(TerrainContext *c)
{
    c->max_hump_peak_m = c->last_hump_height_m = 0.0f;
    terrain_algo_invalidate_window(c);
}
