/*
 *	obstacle_profile.h
 *	Kernel-free obstacle profile and scan state. Rays are measured with the
 *	car stationary; angles are relative to forward, positive to the left.
 *
 *	Scan state (writer: Obstacle) is one 3-bit encoded value. The profile is
 *	larger than 32 bits and is copied under a mutex (robot_snapshot.h).
 *	Field widths are PROVISIONAL until #28/#54 sign off the S5 rows.
 */

#ifndef OBSTACLE_PROFILE_H
#define OBSTACLE_PROFILE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define S5_SCAN_CAPACITY 9u
#define S5_SCAN_STATE_BITS 3u

typedef enum
{
    S5_SCAN_IDLE,
    S5_SCAN_SETTLING,
    S5_SCAN_COARSE,
    S5_SCAN_FINE,
    S5_SCAN_DONE,
    S5_SCAN_FAILED,
    S5_SCAN_CANCELLED,
    S5_SIDE_WATCH,		/* servo held toward the obstacle while driving past */
    S5_SCAN_STATE_COUNT		/* 8: exactly fills 3 bits */
} s5_scan_state_t;

/* Each ray: 8-bit signed proposed servo angle (-75..75), 1-bit validity,
   16-bit range. Attribute packed prevents unwanted byte padding; this
   array is task-private, and the wider profile is mutex-copied. */
typedef struct __attribute__((packed))
{
    int8_t angle_deg;
    uint8_t valid : 1;
    uint16_t range_mm;
} ObstacleRay;

/* Closest angle and validity occupy the former angle/alignment bytes. All six geometrical distances
   and coordinates remain 16-bit: D05 has no validated tighter sonar bound. */
typedef struct
{
    int8_t closest_angle_deg;
    uint8_t valid : 1;
    uint16_t closest_mm;
    uint16_t width_mm;
    uint16_t left_clearance_mm;
    uint16_t right_clearance_mm;
    int16_t obstacle_forward_mm;
    int16_t obstacle_left_mm;
} ObstacleProfile;

_Static_assert(sizeof(ObstacleRay) == 4u, "packed obstacle ray must use 32 bits");
_Static_assert(sizeof(ObstacleProfile) == 14u, "obstacle profile layout changed unexpectedly");

/* Public construction/decoding helpers: callers do not mask packed fields. */
static inline ObstacleRay s5_ray_make(int16_t angle_deg, uint16_t range_mm, bool valid)
{
    ObstacleRay ray = {0};
    if (angle_deg < -SERVO_MAX_DEG || angle_deg > SERVO_MAX_DEG)
    {
        ray.valid = false;
        ray.angle_deg = 0;
        ray.range_mm = 0u;
    }
    else
    {
        ray.angle_deg = (int8_t)angle_deg;
        ray.valid = valid;
        ray.range_mm = range_mm;
    }
    return ray;
}

static inline uint32_t s5_side_watch_pack(bool on, int16_t angle_deg)
{
    if (angle_deg < -SERVO_MAX_DEG) angle_deg = -SERVO_MAX_DEG;
    if (angle_deg > SERVO_MAX_DEG) angle_deg = SERVO_MAX_DEG;
    /* Atomic word: bit 0 enabled, bits 1..8 signed 8-bit angle. */
    return (on ? 1u : 0u) | (((uint32_t)((uint8_t)angle_deg)) << 1u);
}

static inline int16_t s5_side_watch_angle(uint32_t word)
{
    unsigned value = (word >> 1u) & 0xFFu;
    return (int16_t)(value < 128u ? (int)value : (int)value - 256);
}

_Static_assert(S5_SCAN_STATE_COUNT == (1 << S5_SCAN_STATE_BITS), "scan state must fill 3 bits");

/* Builds the profile from 5..S5_SCAN_CAPACITY rays; false (out unchanged)
   when no ray echoed. Rays without an echo are skipped; a side whose
   outermost ray has no echo is unknown and reports zero clearance. */
bool s5_profile_build(const ObstacleRay *rays, unsigned count, ObstacleProfile *out);

/* Fine angles around the nearest valid coarse ray (at most `max`, within the
   sweep limit, excluding angles already measured). Returns how many. */
unsigned s5_fine_angles(const ObstacleRay *coarse, unsigned count, int16_t *out, unsigned max);

/* Encode/decode the scan state. Decode maps an out-of-range code to
   S5_SCAN_FAILED; encode rejects one (returns false). */
bool s5_scan_state_pack(s5_scan_state_t state, uint32_t *word);
s5_scan_state_t s5_scan_state_unpack(uint32_t code);

/* A scan runs only while stationary and on time; cancel wins. */
s5_scan_state_t s5_scan_guard(bool cancel, bool expired, bool stationary);

#endif /* OBSTACLE_PROFILE_H */
