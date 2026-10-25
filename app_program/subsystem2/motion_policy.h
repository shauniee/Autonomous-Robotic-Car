#ifndef MOTION_POLICY_H
#define MOTION_POLICY_H
#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
typedef struct {
    float left, right;
    uint32_t sampled_ms;
    bool enabled;
} MotionRequest;
/* Pure acceptance gate. A cached request never renews its acquisition time. */
bool motion_request_valid(MotionRequest request, bool stopped, uint32_t now);
#endif
