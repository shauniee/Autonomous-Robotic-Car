#include "motion_policy.h"
#include <math.h>

bool motion_duties_valid(float left, float right)
{
    return isfinite(left) && isfinite(right) &&
        (left >= 0.0f) && (right >= 0.0f) &&
        (left <= MOTION_MAX_DUTY) && (right <= MOTION_MAX_DUTY);
}

bool motion_request_valid(MotionRequest request, bool stopped, uint32_t now)
{
    return !stopped && request.enabled &&
        ((uint32_t)(now - request.sampled_ms) <= MOTION_EXPIRY_MS) &&
        (request.velocity_mode ||
         motion_duties_valid(request.left, request.right));
}

uint32_t motion_stop_generation(uint32_t state)
{
    return state >> 1;
}

bool motion_stop_latched(uint32_t state)
{
    return (state & MOTION_STOP_BIT) != 0u;
}

uint32_t motion_stop_next(uint32_t state)
{
    uint32_t generation = motion_stop_generation(state);
    if (generation >= MOTION_MAX_GENERATION)
    {
        return UINT32_MAX;
    }
    return ((generation + 1u) << 1) | MOTION_STOP_BIT;
}

bool motion_request_current(MotionRequest request, uint32_t state,
    uint32_t now)
{
    return (request.generation > 0u) &&
        (request.generation == motion_stop_generation(state)) &&
        motion_request_valid(request, motion_stop_latched(state), now);
}

bool motion_reset_allowed(uint32_t state, uint32_t expected_generation,
    uint32_t brake_ack_generation, bool output_braked, bool driver_ok)
{
    return motion_stop_latched(state) && driver_ok && output_braked &&
        (expected_generation > 0u) &&
        (expected_generation < MOTION_MAX_GENERATION) &&
        (expected_generation == motion_stop_generation(state)) &&
        (expected_generation == brake_ack_generation);
}

bool motion_publication_allowed(MotionRequest request, uint32_t state,
    uint32_t reset_ack_generation, bool driver_ok)
{
    if (!request.enabled)
    {
        return true;
    }
    return driver_ok && !motion_stop_latched(state) &&
        (request.generation > 0u) &&
        (request.generation == motion_stop_generation(state)) &&
        (request.generation == reset_ack_generation);
}
