#include "motion_policy.h"
#include <math.h>
bool motion_request_valid(MotionRequest r, bool stopped, uint32_t now) {
    return !stopped && r.enabled && (uint32_t)(now-r.sampled_ms) <= MOTION_EXPIRY_MS &&
        isfinite(r.left) && isfinite(r.right) && r.left >= 0 && r.right >= 0 &&
        r.left <= MOTION_MAX_DUTY && r.right <= MOTION_MAX_DUTY;
}
