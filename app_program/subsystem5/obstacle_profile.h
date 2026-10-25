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

typedef struct
{
    int16_t angle_deg;
    uint16_t range_mm;
    bool valid;
} ObstacleRay;

typedef struct
{
    int16_t closest_angle_deg;
    uint16_t closest_mm;		/* distance from the reference point to the nearest ray */
    uint16_t width_mm;		/* extent across the obstacle within the car's depth */
    uint16_t left_clearance_mm;
    uint16_t right_clearance_mm;
    int16_t obstacle_forward_mm;	/* nearest point, forward of the reference point */
    int16_t obstacle_left_mm;	/* nearest point, left of the reference point */
    bool valid;
} ObstacleProfile;

_Static_assert(S5_SCAN_STATE_COUNT == (1 << S5_SCAN_STATE_BITS), "scan state must fill 3 bits");

/* Builds the profile; false (out unchanged) unless there are 5..S5_SCAN_CAPACITY
   rays, all valid, finite and within the sweep limit. */
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
