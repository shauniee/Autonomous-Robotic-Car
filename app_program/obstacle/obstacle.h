/* Obstacle public interface: range filter, profile estimation and task API. */
#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/robot_data.h"

/*
 * Kernel-free ultrasonic ranging helpers: pulse width to range, the packed
 * front-range word and the no-echo counter.
 *
 * Front-range word (writer: Obstacle). Layout is PROVISIONAL until the Obstacle
 * rows of DATA_REPRESENTATION are signed off (#28, #54):
 *   bits  0-12  range in mm, 0..8190; 8191 is the invalid code
 *   bit  13     valid (range is a real echo)
 *   bits 14-15  spare, zero. The sample time is the 32-bit sampled_ms.
 * No echo is unknown, never clear space: valid is 0 and range is invalid.
 */

#define ECHO_RANGE_BITS 13u
#define ECHO_RANGE_INVALID 8191u

/* Range in mm for a pulse width; ECHO_RANGE_INVALID outside the usable window. */
static inline uint32_t echo_range_mm(uint32_t pulse_us)
{
    if (pulse_us < SONAR_MIN_PULSE_US || pulse_us >= SONAR_TIMEOUT_US) {
        return ECHO_RANGE_INVALID;
    }
    uint32_t mm = (pulse_us * SONAR_MM_PER_MS_X10) / 10000u;	/* us -> ms x10 scale */
    return mm >= ECHO_RANGE_INVALID ? ECHO_RANGE_INVALID : mm;
}

static inline uint16_t echo_pack(uint32_t range_mm)
{
    bool valid = range_mm < ECHO_RANGE_INVALID;
    return (uint16_t)((valid ? range_mm : ECHO_RANGE_INVALID) |
        ((valid ? 1u : 0u) << 13));
}

static inline bool echo_valid(uint32_t word) { return rd_field_get(word, 13u, 1u) != 0u; }
static inline uint32_t echo_range(uint32_t word) { return rd_field_get(word, 0u, ECHO_RANGE_BITS); }
/* Consecutive misses, saturating at SONAR_NO_ECHO_LIMIT (2 bits). */
#define ECHO_MISS_BITS 2u
_Static_assert(SONAR_NO_ECHO_LIMIT <= 3u, "miss count fits 2 bits");
static inline uint8_t echo_misses_next(uint8_t misses, bool got_echo)
{
    return got_echo ? 0u :
        (misses < SONAR_NO_ECHO_LIMIT ? (uint8_t)(misses + 1u) : misses);
}

/* True while the no-echo fault should be set. */
static inline bool echo_no_echo_fault(uint8_t misses)
{
    return misses >= SONAR_NO_ECHO_LIMIT;
}

/* Coherent acquisition evidence. */
typedef struct
{
    uint32_t sampled_ms;
    uint32_t sequence; /* 0 before the first reading; change counter */
    uint16_t wheel_count[2]; /* enc_count_t at range completion */
    uint16_t word; /* front-range word, 14 bits used */
} obstacle_range_sample_t;
_Static_assert(sizeof(obstacle_range_sample_t) == 16u, "range evidence ABI");
_Static_assert(ENC_COUNT_BITS <= 16u, "wheel counts fit 16 bits");
_Static_assert(SERVO_MAX_DEG <= INT8_MAX, "scan angles fit int8_t");

static inline bool obstacle_range_sample_fresh(const obstacle_range_sample_t *sample,
    uint32_t now, uint32_t maximum_age_ms)
{
    return sample != 0 && sample->sequence != 0u && echo_valid(sample->word) &&
        (uint32_t)(now - sample->sampled_ms) <= maximum_age_ms;
}

/*
 * Kernel-free obstacle profile and scan state. Rays are measured with the
 * car stationary; angles are relative to forward, positive to the left.
 *
 * Scan state (writer: Obstacle) is one 3-bit encoded value. The profile is
 * larger than 32 bits and is copied under a mutex.
 * Field widths are PROVISIONAL until #28/#54 sign off the Obstacle rows.
 */

#define OBSTACLE_SCAN_CAPACITY 15u /* 13 coarse rays, one fine ray per edge */
#define OBSTACLE_SCAN_STATE_BITS 3u

typedef enum
{
    OBSTACLE_SCAN_IDLE,
    OBSTACLE_SCAN_SETTLING,
    OBSTACLE_SCAN_COARSE,
    OBSTACLE_SCAN_FINE,
    OBSTACLE_SCAN_DONE,
    OBSTACLE_SCAN_FAILED,
    OBSTACLE_SCAN_CANCELLED,
    OBSTACLE_SIDE_WATCH,		/* servo held toward the obstacle while driving past */
    OBSTACLE_SCAN_STATE_COUNT		/* 8: exactly fills 3 bits */
} obstacle_scan_state_t;

/* Each ray: 8-bit signed proposed servo angle (bounded by SERVO_MAX_DEG), 1-bit validity,
   13-bit range, as the echo word. Attribute packed prevents unwanted byte padding; this
   array is task-private, and the wider profile is mutex-copied. */
typedef struct __attribute__((packed))
{
    int8_t angle_deg;
    uint16_t range_mm : 13;
    uint16_t valid : 1;
} ObstacleRay;

/* Distances saturate at OBSTACLE_PROFILE_MAX_MM: past it a side has more room than
   any bypass uses (BYPASS_MAX_OFFSET_MM), so the exact value changes nothing. */
#define OBSTACLE_PROFILE_MAX_MM 1023u
_Static_assert(BYPASS_MAX_OFFSET_MM < OBSTACLE_PROFILE_MAX_MM, "profile cap below bypass reach");

typedef struct __attribute__((packed))
{
    int8_t closest_angle_deg : 7;
    uint8_t valid : 1;
    uint16_t closest_mm : 10;
    uint16_t width_mm : 10;
    uint16_t left_clearance_mm : 10;
    uint16_t right_clearance_mm : 10;
    int16_t obstacle_forward_mm : 11;
    int16_t obstacle_left_mm : 11;
} ObstacleProfile;

_Static_assert(sizeof(ObstacleRay) == 3u, "packed obstacle ray must use 24 bits");
_Static_assert(sizeof(ObstacleProfile) == 9u, "obstacle profile layout changed unexpectedly");

static inline uint16_t obstacle_profile_mm(uint32_t mm)
{
    return (uint16_t)(mm > OBSTACLE_PROFILE_MAX_MM ? OBSTACLE_PROFILE_MAX_MM : mm);
}

/* Public construction/decoding helpers: callers do not mask packed fields. */
static inline ObstacleRay obstacle_ray_make(int16_t angle_deg, uint16_t range_mm, bool valid)
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
        ray.range_mm = range_mm >= ECHO_RANGE_INVALID ? ECHO_RANGE_INVALID : range_mm;
    }
    return ray;
}

static inline uint32_t obstacle_side_watch_pack(bool on, int16_t angle_deg)
{
    if (angle_deg < -SERVO_LOOK_DEG)
    {
        angle_deg = -SERVO_LOOK_DEG;
    }
    if (angle_deg > SERVO_LOOK_DEG)
    {
        angle_deg = SERVO_LOOK_DEG;
    }
    /* Atomic word: bit 0 enabled, bits 1..8 signed 8-bit angle. */
    return (on ? 1u : 0u) | (((uint32_t)((uint8_t)angle_deg)) << 1u);
}

static inline int16_t obstacle_side_watch_angle(uint32_t word)
{
    unsigned value = (word >> 1u) & 0xFFu;
    return (int16_t)(value < 128u ? (int)value : (int)value - 256);
}

_Static_assert(OBSTACLE_SCAN_STATE_COUNT == (1 << OBSTACLE_SCAN_STATE_BITS), "scan state must fill 3 bits");

/* Encode/decode the scan state. Decode maps an out-of-range code to
   OBSTACLE_SCAN_FAILED; encode rejects one (returns false). */
bool obstacle_scan_state_pack(obstacle_scan_state_t state, uint32_t *word);
obstacle_scan_state_t obstacle_scan_state_unpack(uint32_t code);

/* Scan word: state in bits 0..2, scan id (0..15) in bits 3..6. */
static inline uint32_t obstacle_scan_id_pack(uint8_t id)
{
    return (uint32_t)id << OBSTACLE_SCAN_STATE_BITS;
}

static inline uint8_t obstacle_scan_id(uint32_t word)
{
    return (uint8_t)((word >> OBSTACLE_SCAN_STATE_BITS) & 0x0Fu);
}

/* A scan runs only while stationary and on time; cancel wins. */
obstacle_scan_state_t obstacle_scan_guard(bool cancel, bool expired, bool stationary);

/* Fixed-storage repeated ranging; no clocks, waits, hardware or kernel calls. */

#define OBSTACLE_RANGE_GROUP_CAPACITY 5u

typedef struct
{
    uint16_t values[OBSTACLE_RANGE_GROUP_CAPACITY];
    uint16_t attempts : 3;
    uint16_t accepted : 3;
    uint16_t requested : 3;
    uint16_t minimum : 3;
} obstacle_range_group_t;

/* Odd requested count 1..5; minimum valid count is a strict majority. */
bool obstacle_range_group_begin(obstacle_range_group_t *group, unsigned requested,
    unsigned minimum);
/* Exactly one attempt; false for an uninitialized/already-complete group. */
bool obstacle_range_group_push(obstacle_range_group_t *group, uint32_t range_mm);
bool obstacle_range_group_complete(const obstacle_range_group_t *group);
/* True only for a complete group with its minimum valid count. */
bool obstacle_range_group_median(const obstacle_range_group_t *group, uint16_t *range_mm);

_Static_assert(sizeof(obstacle_range_group_t) == 12u, "range group layout changed");

/* Qualified profile from ordered/unsorted stationary sonar rays. */

typedef struct
{
    uint16_t depth_enter_mm;
    uint16_t depth_exit_mm;
    uint16_t background_separation_mm;
    uint16_t range_uncertainty_mm; /* UINT16_MAX = unknown */
    uint16_t planning_width_mm;
    uint8_t adjacent_rays;
    uint8_t maximum_gap_deg;
} obstacle_profile_config_t;

#define OBSTACLE_UNCERTAINTY_UNKNOWN 2047u
_Static_assert(OBSTACLE_PROFILE_MAX_MM < OBSTACLE_UNCERTAINTY_UNKNOWN,
    "unknown uncertainty is outside the measured range");

typedef struct
{
    /* 0..OBSTACLE_PROFILE_MAX_MM; OBSTACLE_UNCERTAINTY_UNKNOWN = unknown */
    uint16_t width_model_uncertainty_mm : 11;
    uint16_t planning_width_mm : 10; /* 0..OBSTACLE_PROFILE_MAX_MM */
    uint16_t foreground_rays : 4;
    uint16_t free_left_rays : 4;
    uint16_t free_right_rays : 4;
    uint16_t left_edge_valid : 1;
    uint16_t right_edge_valid : 1;
} obstacle_profile_evidence_t;

/* Requires a valid centre ray and adjacent foreground evidence, bracketed
   on both sides by valid background rays. Invalid/no-echo never means free.
   Optional background is a known empty-scene reference with matching angles.
   Configured uncertainty is a model input, never a calibrated sensor rating.
   False leaves both outputs unchanged. No waits or dynamic allocation. */
bool obstacle_profile_estimate(const ObstacleRay *rays, unsigned count,
    const ObstacleRay *background, const obstacle_profile_config_t *config,
    ObstacleProfile *out, obstacle_profile_evidence_t *evidence);

/* Up to two bounded refinements per supported object/background edge. */
unsigned obstacle_profile_fine_angles(const ObstacleRay *coarse, unsigned count,
    const obstacle_profile_config_t *config, int8_t *out, unsigned capacity);

/* Jia Jing: obstacle. Front ultrasonic range on the GPIO dispatcher; servo scan follows in #49. */

ER obstacle_init(void);
void obstacle_step(uint32_t now_ms);
/* Mission-only (same task as obstacle_step): full timestamps prevent the packed
   16-bit display time from reviving a frozen reading after 65536 ms. */
ER obstacle_range_samples(obstacle_range_sample_t *front, obstacle_range_sample_t *side);
/* Optional cross-core reporting mirror; nonwaiting coherent copy. */
ER obstacle_range_try_copy(obstacle_range_sample_t *front, obstacle_range_sample_t *side);

typedef struct
{
    ObstacleProfile profile;
    obstacle_profile_evidence_t evidence;
    uint32_t sampled_ms;
} obstacle_profile_snapshot_t;
_Static_assert(sizeof(obstacle_profile_snapshot_t) == 20u, "profile snapshot ABI");
ER obstacle_profile_snapshot(obstacle_profile_snapshot_t *out);

/* Request a coarse-then-fine scan (id 0..15). Honoured only while the car is
   stationary; any movement, cancel or the deadline fails it. Mission starts it
   and polls obstacle_scan_word(). Single caller (Mission). */
/* keep_narrower: leave the current profile in place unless this sweep reads
   narrower. */
void obstacle_scan_request(uint8_t scan_id, bool keep_narrower);
void obstacle_scan_cancel(void);

/* Scan word (writer: Obstacle): bits 0-2 obstacle_scan_state_t, bits 3-6 scan id. */
uint32_t obstacle_scan_word(void);

/* Latest finished profile, copied under a mutex. E_OK when the last scan
   completed; E_OBJ if none yet. */
ER obstacle_profile_try_copy(ObstacleProfile *out);

/* Side-watch for the bypass leg: hold the servo at angle_deg (relative to
   forward, + left) and range it every release. Read the side sample through
   obstacle_range_samples(). Turning it off recentres the servo. Mission-only caller. */
void obstacle_side_watch(bool on, int16_t angle_deg);

#endif /* OBSTACLE_H */
