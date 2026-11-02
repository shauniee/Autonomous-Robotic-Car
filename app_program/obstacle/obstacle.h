/* Jia Jing: obstacle. Front ultrasonic range on the GPIO dispatcher; servo scan follows in #49. */
#ifndef OBSTACLE_H
#define OBSTACLE_H
#include "../app.h"
#include "obstacle_profile.h"

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
_Static_assert(sizeof(obstacle_profile_snapshot_t) == 24u, "profile snapshot ABI");
ER obstacle_profile_snapshot(obstacle_profile_snapshot_t *out);

/* Request a coarse-then-fine scan (id 0..15). Honoured only while the car is
   stationary; any movement, cancel or the deadline fails it. Mission starts it
   and polls obstacle_scan_word(). Single caller (Mission). */
void obstacle_scan_request(uint8_t scan_id);
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
#endif
