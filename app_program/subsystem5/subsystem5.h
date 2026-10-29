/* Jia Jing: obstacle. Front ultrasonic range on the GPIO dispatcher; servo scan follows in #49. */
#ifndef SUBSYSTEM5_H
#define SUBSYSTEM5_H
#include "../app.h"
#include "echo_range.h"
#include "obstacle_profile.h"

ER subsystem5_init(void);
void s5_step(uint32_t now_ms);

/* Front-range word (see echo_range.h): one load, any task. 0 before the first
   reading; a word with valid = 0 means no echo, never clear space. */
uint32_t s5_front_word(void);

/* Request a coarse-then-fine scan (id 0..15). Honoured only while the car is
   stationary; any movement, cancel or the deadline fails it. Mission starts it
   and polls s5_scan_word(). Single caller (Mission). */
void s5_scan_request(uint8_t scan_id);
void s5_scan_cancel(void);

/* Scan word (writer: Obstacle): bits 0-2 s5_scan_state_t, bits 3-6 scan id. */
uint32_t s5_scan_word(void);

/* Latest finished profile, copied under a mutex. E_OK when the last scan
   completed; E_OBJ if none yet. */
ER s5_profile_copy(ObstacleProfile *out);
ER s5_profile_try_copy(ObstacleProfile *out);

/* Side-watch for the bypass leg: hold the servo at angle_deg (relative to
   forward, + left) and range it every release. Poll s5_side_word() (front
   word layout). Turning it off recentres the servo. Single caller (Mission). */
void s5_side_watch(bool on, int16_t angle_deg);
uint32_t s5_side_word(void);
#endif
