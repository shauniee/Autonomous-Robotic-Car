#include "mission_guards.h"
#include <math.h>
bool mission_sensors_ready(uint32_t now, const S3IrSnapshot *ir,
                           const MotionFeedback *motion,
                           const TerrainSnapshot *terrain)
{
    return ir != NULL && motion != NULL && terrain != NULL &&
           ir->normalized_valid && ir->capture_kind == IR_CAPTURE_NONE &&
           now - ir->sampled_ms <= 60u && motion->valid &&
           now - motion->sampled_ms <= 60u && terrain->terrain_valid &&
           terrain->calibrated && now - terrain->timestamp_ms <= 250u &&
           terrain->state != TERRAIN_STATE_IMPACT;
}
uint32_t mission_broad_observe(MissionDwell *d, const S3IrSnapshot *ir,
                               uint32_t now)
{
    if (!ir->normalized_valid || ir->line != IR_LINE_BROAD_BLACK ||
        now - ir->sampled_ms > 60u)
    {
        *d = (MissionDwell){0};
        return 0u;
    }
    if (d->count != 0u && ir->sequence == d->sequence)
    {
        return d->count >= 2u ? d->last_ms - d->first_ms : 0u;
    }
    if (d->count == 0u || ir->sequence - d->sequence != 1u ||
        ir->sampled_ms - d->last_ms == 0u || ir->sampled_ms - d->last_ms > 60u)
    {
        *d = (MissionDwell){.first_ms = ir->sampled_ms, .count = 1u};
    }
    else if (d->count < UINT32_MAX)
    {
        ++d->count;
    }
    d->last_ms = ir->sampled_ms;
    d->sequence = ir->sequence;
    return d->count >= 2u ? d->last_ms - d->first_ms : 0u;
}
bool mission_junction_check(const RobotConfig *c, float distance,
                            uint32_t held_ms)
{
    return c != NULL && c->course_confirmed && isfinite(distance) &&
           distance >= c->junction_min_m && distance <= c->junction_max_m &&
           held_ms >= c->junction_hold_ms;
}
bool mission_finish_check(const RobotConfig *c, uint32_t actions,
                          float distance, uint32_t held_ms)
{
    return c != NULL && c->course_confirmed &&
           c->finish_rule == FINISH_BROAD_BLACK &&
           actions == c->finish_after_actions && isfinite(distance) &&
           distance >= c->finish_min_m && held_ms >= c->finish_black_ms;
}
float mission_pivot_radius(const RobotConfig *c)
{
    return hypotf(c->body_width_m, c->body_length_m) * 0.5f + c->clearance_m;
}
float mission_obstacle_trigger(const RobotConfig *c)
{
    return mission_pivot_radius(c) + c->braking_headroom_m - c->sonar_forward_m;
}
