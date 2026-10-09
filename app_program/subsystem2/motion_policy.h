#ifndef MOTION_POLICY_H
#define MOTION_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#define MOTION_MAX_DUTY 0.35f
#define MOTION_EXPIRY_MS 60u
#define MOTION_STOP_BIT 1u
#define MOTION_BOOT_STATE 3u /* Generation 1, stopped. */
#define MOTION_MAX_GENERATION (UINT32_MAX >> 1)

typedef struct
{
    float left;
    float right;
    uint32_t sampled_ms;
    bool enabled;
    uint32_t generation;
} MotionRequest;

bool motion_duties_valid(float left, float right);
/* A cached request never renews its acquisition time. */
bool motion_request_valid(MotionRequest request, bool stopped, uint32_t now);
bool motion_request_current(MotionRequest request, uint32_t state,
    uint32_t now);
/* An enabled target cannot queue while stopped or before reset is observed. */
bool motion_publication_allowed(MotionRequest request, uint32_t state,
    uint32_t reset_ack_generation, bool driver_ok);
uint32_t motion_stop_generation(uint32_t state);
bool motion_stop_latched(uint32_t state);
/* Generation exhaustion remains stopped until reboot; tokens never wrap. */
uint32_t motion_stop_next(uint32_t state);
bool motion_reset_allowed(uint32_t state, uint32_t expected_generation,
    uint32_t brake_ack_generation, bool output_braked, bool driver_ok);

#endif
