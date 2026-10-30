/* Jia Jing: obstacle. Front ultrasonic range on the GPIO dispatcher; servo scan follows in #49. */
#ifndef SUBSYSTEM5_H
#define SUBSYSTEM5_H
#include "../app.h"
#include "echo_range.h"
#include "obstacle_profile.h"
#include "profile_edges.h"

ER subsystem5_init(void);
void s5_step(uint32_t now_ms);
/* Mission-only (same task as s5_step): full timestamps prevent the packed
   16-bit display time from reviving a frozen reading after 65536 ms. */
ER s5_range_samples(s5_range_sample_t *front, s5_range_sample_t *side);
/* Optional cross-core reporting mirror; nonwaiting coherent copy. */
ER s5_range_try_copy(s5_range_sample_t *front, s5_range_sample_t *side);

typedef struct
{
    ObstacleProfile profile;
    s5_profile_evidence_t evidence;
    uint32_t sampled_ms;
} s5_profile_snapshot_t;
_Static_assert(sizeof(s5_profile_snapshot_t) == 24u, "profile snapshot ABI");
ER s5_profile_snapshot(s5_profile_snapshot_t *out);

/* Request a coarse-then-fine scan (id 0..15). Honoured only while the car is
   stationary; any movement, cancel or the deadline fails it. Mission starts it
   and polls s5_scan_word(). Single caller (Mission). */
void s5_scan_request(uint8_t scan_id);
void s5_scan_cancel(void);

/* Scan word (writer: Obstacle): bits 0-2 s5_scan_state_t, bits 3-6 scan id. */
uint32_t s5_scan_word(void);

/* Latest finished profile, copied under a mutex. E_OK when the last scan
   completed; E_OBJ if none yet. */
ER s5_profile_try_copy(ObstacleProfile *out);

/* Side-watch for the bypass leg: hold the servo at angle_deg (relative to
   forward, + left) and range it every release. Read the side sample through
   s5_range_samples(). Turning it off recentres the servo. Mission-only caller. */
void s5_side_watch(bool on, int16_t angle_deg);
#endif
