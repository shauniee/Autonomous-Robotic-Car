#include "obstacle_profile.h"
#include <math.h>
#include <stddef.h>

/* ---- Scan and side-watch words ---- */
bool obstacle_scan_state_pack(obstacle_scan_state_t state, uint32_t *word)
{
    if ((unsigned)state >= OBSTACLE_SCAN_STATE_COUNT || word == 0)
    {
        return false;
    }
    *word = (uint32_t)state;
    return true;
}

obstacle_scan_state_t obstacle_scan_state_unpack(uint32_t code)
{
    return code < OBSTACLE_SCAN_STATE_COUNT ? (obstacle_scan_state_t)code : OBSTACLE_SCAN_FAILED;
}

obstacle_scan_state_t obstacle_scan_guard(bool cancel, bool expired, bool stationary)
{
    return cancel ? OBSTACLE_SCAN_CANCELLED : ((expired || !stationary) ? OBSTACLE_SCAN_FAILED : OBSTACLE_SCAN_COARSE);
}

/* ---- Repeated-ranging filter ---- */
bool obstacle_range_group_begin(obstacle_range_group_t *group, unsigned requested,
    unsigned minimum)
{
    if (group == NULL || requested == 0u ||
        requested > OBSTACLE_RANGE_GROUP_CAPACITY || (requested & 1u) == 0u ||
        minimum == 0u || minimum > requested || minimum <= requested / 2u)
    {
        return false;
    }
    *group = (obstacle_range_group_t){.requested = (uint16_t)requested,
        .minimum = (uint16_t)minimum};
    return true;
}

bool obstacle_range_group_complete(const obstacle_range_group_t *group)
{
    return group != NULL && group->requested != 0u &&
        group->attempts == group->requested;
}

bool obstacle_range_group_push(obstacle_range_group_t *group, uint32_t range_mm)
{
    if (group == NULL || group->requested == 0u ||
        group->requested > OBSTACLE_RANGE_GROUP_CAPACITY ||
        group->attempts >= group->requested)
    {
        return false;
    }
    ++group->attempts;
    if (range_mm > 0u && range_mm < ECHO_RANGE_INVALID)
    {
        group->values[group->accepted++] = (uint16_t)range_mm;
    }
    return true;
}

bool obstacle_range_group_median(const obstacle_range_group_t *group, uint16_t *range_mm)
{
    if (range_mm == NULL || !obstacle_range_group_complete(group) ||
        group->minimum == 0u || group->accepted < group->minimum ||
        group->accepted > OBSTACLE_RANGE_GROUP_CAPACITY)
    {
        return false;
    }
    uint16_t sorted[OBSTACLE_RANGE_GROUP_CAPACITY];
    for (unsigned i = 0u; i < group->accepted; ++i)
    {
        unsigned at = i;
        while (at > 0u && sorted[at - 1u] > group->values[i])
        {
            sorted[at] = sorted[at - 1u];
            --at;
        }
        sorted[at] = group->values[i];
    }
    unsigned middle = group->accepted / 2u;
    if ((group->accepted & 1u) != 0u)
    {
        *range_mm = sorted[middle];
    }
    else
    {
        *range_mm = (uint16_t)(((uint32_t)sorted[middle - 1u] +
            sorted[middle] + 1u) / 2u);
    }
    return true;
}

/* ---- Profile edge estimation ---- */
#define OBSTACLE_PROFILE_DEG_TO_RAD 0.0174532925f

static bool ray_valid(const ObstacleRay *ray)
{
    return ray->valid && ray->range_mm != 0u && ray->range_mm < ECHO_RANGE_INVALID &&
        ray->angle_deg >= -SERVO_MAX_DEG && ray->angle_deg <= SERVO_MAX_DEG;
}

static uint16_t distance_code(float value)
{
    return value <= 0.0f ? 0u :
        (value >= 65535.0f ? UINT16_MAX : (uint16_t)(value + 0.5f));
}

static int16_t coordinate_code(float value)
{
    return value <= -32768.0f ? INT16_MIN :
        (value >= 32767.0f ? INT16_MAX : (int16_t)lroundf(value));
}

static float plane_left(float forward, float angle)
{
    return OBSTACLE_SONAR_LEFT_MM + forward * tanf(angle * OBSTACLE_PROFILE_DEG_TO_RAD);
}

static bool config_valid(const obstacle_profile_config_t *config)
{
    return config != NULL && config->depth_exit_mm >= config->depth_enter_mm &&
        config->adjacent_rays >= 2u && config->adjacent_rays <= OBSTACLE_SCAN_CAPACITY &&
        config->maximum_gap_deg > 0u && config->maximum_gap_deg <= SERVO_MAX_DEG &&
        config->planning_width_mm > 0u;
}

bool obstacle_profile_estimate(const ObstacleRay *rays, unsigned count,
    const ObstacleRay *background, const obstacle_profile_config_t *config,
    ObstacleProfile *out, obstacle_profile_evidence_t *evidence)
{
    if (rays == NULL || out == NULL || evidence == NULL ||
        !config_valid(config) || count < 5u || count > OBSTACLE_SCAN_CAPACITY)
    {
        return false;
    }
    unsigned order[OBSTACLE_SCAN_CAPACITY];
    for (unsigned i = 0u; i < count; ++i)
    {
        if (rays[i].angle_deg < -SERVO_MAX_DEG || rays[i].angle_deg > SERVO_MAX_DEG ||
            (background != NULL && background[i].angle_deg != rays[i].angle_deg))
        {
            return false;
        }
        unsigned at = i;
        while (at > 0u && rays[order[at - 1u]].angle_deg > rays[i].angle_deg)
        {
            order[at] = order[at - 1u];
            --at;
        }
        order[at] = i;
    }
    unsigned centre = count;
    for (unsigned i = 0u; i < count; ++i)
    {
        if (i != 0u && rays[order[i - 1u]].angle_deg == rays[order[i]].angle_deg)
        {
            return false;
        }
        if (rays[order[i]].angle_deg == 0 && ray_valid(&rays[order[i]]))
        {
            centre = i;
        }
    }
    if (centre == count)
    {
        return false;
    }
    float front = rays[order[centre]].range_mm;
    bool foreground[OBSTACLE_SCAN_CAPACITY] = {false};
    bool known[OBSTACLE_SCAN_CAPACITY] = {false};
    /* Walk outwards from the forward target. A gap/invalid ray breaks its
       continuity. Exit hysteresis keeps marginal neighbours in the region;
       a subsequent separate region needs the stricter enter threshold. */
    for (int direction = -1; direction <= 1; direction += 2)
    {
        bool inside = true;
        for (int index = (int)centre; index >= 0 && index < (int)count;
             index += direction)
        {
            const ObstacleRay *ray = &rays[order[index]];
            known[index] = ray_valid(ray);
            float forward = (float)ray->range_mm *
                cosf((float)ray->angle_deg * OBSTACLE_PROFILE_DEG_TO_RAD);
            unsigned threshold = inside ? config->depth_exit_mm : config->depth_enter_mm;
            bool target = known[index] && forward <= front + (float)threshold;
            if (background != NULL && target)
            {
                const ObstacleRay *reference = &background[order[index]];
                known[index] = ray_valid(reference);
                target = known[index] &&
                    (uint32_t)ray->range_mm + config->background_separation_mm <= reference->range_mm;
            }
            foreground[index] = target;
            inside = target;
        }
    }
    if (!foreground[centre])
    {
        return false;
    }
    unsigned low = centre, high = centre;
    while (low > 0u && foreground[low - 1u] &&
        rays[order[low]].angle_deg - rays[order[low - 1u]].angle_deg <= config->maximum_gap_deg)
    {
        --low;
    }
    while (high + 1u < count && foreground[high + 1u] &&
        rays[order[high + 1u]].angle_deg - rays[order[high]].angle_deg <= config->maximum_gap_deg)
    {
        ++high;
    }
    if (high - low + 1u < config->adjacent_rays || low == 0u || high + 1u >= count ||
        !known[low - 1u] || !known[high + 1u] ||
        foreground[low - 1u] || foreground[high + 1u] ||
        rays[order[low]].angle_deg - rays[order[low - 1u]].angle_deg > config->maximum_gap_deg ||
        rays[order[high + 1u]].angle_deg - rays[order[high]].angle_deg > config->maximum_gap_deg)
    {
        return false;
    }
    float right_angle = ((float)rays[order[low - 1u]].angle_deg + rays[order[low]].angle_deg) * 0.5f;
    float left_angle = ((float)rays[order[high + 1u]].angle_deg + rays[order[high]].angle_deg) * 0.5f;
    float right_edge = plane_left(front, right_angle);
    float left_edge = plane_left(front, left_angle);
    float width = left_edge - right_edge;
    float outer_width = plane_left(front, rays[order[high + 1u]].angle_deg) -
        plane_left(front, rays[order[low - 1u]].angle_deg);
    float inner_width = plane_left(front, rays[order[high]].angle_deg) -
        plane_left(front, rays[order[low]].angle_deg);
    float uncertainty = fmaxf(outer_width - width, width - inner_width);
    bool uncertainty_known = config->range_uncertainty_mm != UINT16_MAX;
    if (uncertainty_known)
    {
        uncertainty += 2.0f * config->range_uncertainty_mm /
            cosf((float)SERVO_MAX_DEG * OBSTACLE_PROFILE_DEG_TO_RAD);
    }
    float planning = fmaxf((float)config->planning_width_mm, width + 2.0f * uncertainty);
    float centre_left = (left_edge + right_edge) * 0.5f;
    unsigned free_left = 0u, free_right = 0u;
    unsigned far_left = high, far_right = low;
    for (unsigned i = high + 1u; i < count && known[i] && !foreground[i]; ++i)
    {
        if (rays[order[i]].angle_deg - rays[order[i - 1u]].angle_deg > config->maximum_gap_deg)
        {
            break;
        }
        ++free_left;
        far_left = i;
    }
    for (unsigned i = low; i > 0u && known[i - 1u] && !foreground[i - 1u]; --i)
    {
        if (rays[order[i]].angle_deg - rays[order[i - 1u]].angle_deg > config->maximum_gap_deg)
        {
            break;
        }
        ++free_right;
        far_right = i - 1u;
    }
    *out = (ObstacleProfile){.closest_angle_deg = 0,
        .closest_mm = distance_code(hypotf(front + OBSTACLE_SONAR_FORWARD_MM, OBSTACLE_SONAR_LEFT_MM)),
        .width_mm = distance_code(width),
        .left_clearance_mm = uncertainty_known && free_left >= config->adjacent_rays ?
            distance_code(plane_left(front, rays[order[far_left]].angle_deg) - centre_left - planning * 0.5f) : 0u,
        .right_clearance_mm = uncertainty_known && free_right >= config->adjacent_rays ?
            distance_code(centre_left - planning * 0.5f - plane_left(front, rays[order[far_right]].angle_deg)) : 0u,
        .obstacle_forward_mm = coordinate_code(front + OBSTACLE_SONAR_FORWARD_MM),
        .obstacle_left_mm = coordinate_code(centre_left), .valid = true};
    *evidence = (obstacle_profile_evidence_t){
        .width_model_uncertainty_mm = uncertainty_known ? distance_code(uncertainty) : UINT16_MAX,
        .planning_width_mm = distance_code(planning),
        .foreground_rays = (uint16_t)(high - low + 1u),
        .free_left_rays = (uint16_t)free_left,
        .free_right_rays = (uint16_t)free_right,
        .left_edge_valid = 1u, .right_edge_valid = 1u};
    return true;
}

unsigned obstacle_profile_fine_angles(const ObstacleRay *coarse, unsigned count,
    const obstacle_profile_config_t *config, int16_t *out, unsigned capacity)
{
    if (coarse == NULL || out == NULL || !config_valid(config) ||
        count < 3u || count > OBSTACLE_SCAN_CAPACITY)
    {
        return 0u;
    }
    int centre = -1;
    for (unsigned i = 0u; i < count; ++i)
    {
        if (coarse[i].angle_deg == 0 && ray_valid(&coarse[i]))
        {
            centre = (int)i;
        }
    }
    if (centre < 0)
    {
        return 0u;
    }
    float front = coarse[centre].range_mm;
    unsigned written = 0u;
    /* Refine each object/background transition, rather than spending every
       fine ray near the centre of a wide object. At most two rays per edge. */
    for (int side = -1; side <= 1; side += 2)
    {
        int inside = 0, outside = SERVO_MAX_DEG + 1;
        for (unsigned i = 0u; i < count; ++i)
        {
            int angle = coarse[i].angle_deg * side;
            if (angle <= 0 || !ray_valid(&coarse[i]))
            {
                continue;
            }
            float forward = coarse[i].range_mm *
                cosf(coarse[i].angle_deg * OBSTACLE_PROFILE_DEG_TO_RAD);
            if (forward <= front + config->depth_exit_mm)
            {
                if (angle > inside)
                {
                    inside = angle;
                }
            }
        }
        for (unsigned i = 0u; i < count; ++i)
        {
            int angle = coarse[i].angle_deg * side;
            if (angle > inside && angle < outside && ray_valid(&coarse[i]))
            {
                outside = angle;
            }
        }
        if (outside > SERVO_MAX_DEG)
        {
            continue;
        }
        for (unsigned n = 1u; n <= 2u && written < capacity; ++n)
        {
            int angle = inside + (int)n * OBSTACLE_FINE_STEP_DEG;
            if (angle >= outside)
            {
                break;
            }
            out[written++] = (int16_t)(side * angle);
        }
    }
    return written;
}
