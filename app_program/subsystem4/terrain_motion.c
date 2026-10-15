#include "terrain_motion.h"
#include <math.h>
bool terrain_motion_measure(const MotionFeedback *m, const MotionFeedback *p,
                            bool have_previous, uint32_t now, float *turn,
                            float *acceleration)
{
    *turn = *acceleration = 0.0f;
    uint32_t dt = m->sampled_ms - p->sampled_ms;
    if (!have_previous || !m->valid || !p->valid || now - m->sampled_ms > 60u ||
        m->run_epoch != p->run_epoch || m->sequence == p->sequence ||
        dt == 0u || dt > 60u)
    {
        return false;
    }
    *turn = remainderf(m->yaw_deg - p->yaw_deg, 360.0f) * 1000.0f / (float)dt;
    *acceleration =
        ((m->left_mps + m->right_mps) - (p->left_mps + p->right_mps)) * 500.0f /
        (float)dt;
    return isfinite(*turn) && isfinite(*acceleration);
}
TerrainMovement terrain_movement_classify(const TerrainContext *c,
                                          const MotionFeedback *m, bool valid,
                                          float turn, float acceleration,
                                          const RobotConfig *config)
{
    if (c->state == TERRAIN_STATE_IMPACT)
    {
        return TERRAIN_IMPACT;
    }
    if (valid && m->stationary)
    {
        return TERRAIN_STATIONARY;
    }
    if (valid && fabsf(turn) > config->turn_threshold_deg_s)
    {
        return TERRAIN_TURNING;
    }
    if (c->state == TERRAIN_STATE_CLIMBING)
    {
        return TERRAIN_CLIMB;
    }
    if (c->state == TERRAIN_STATE_DESCENDING)
    {
        return TERRAIN_DESCENT;
    }
    return valid && fabsf(acceleration) > config->acceleration_threshold_mps2
               ? TERRAIN_ACCELERATING
               : TERRAIN_MOVING;
}
