/*
 *	s1_report.h
 *	Kernel-free Reporting logic: the event-log ring and telemetry clamping.
 *	subsystem1.c supplies the sources, mapping hand-off and the
 *	transport; nothing here touches the kernel.
 */

#ifndef S1_REPORT_H
#define S1_REPORT_H

#include <stdbool.h>
#include <stdint.h>
#include "mqtt_frame.h"
#include "../common/robot_config.h"

/* ---- Event log: fixed ring of packed events ------------------------------
   Only the persistent RAM entries are packed; mqtt_event_t is the decoded
   API value and the MQTT wire frame remains the unchanged 11-byte schema v2.

   kind 1..4 needs 3 bits; arg 0..127 needs 7 bits (height code is
   the widest arg). The resulting 10-bit word has six unused high bits.
   time, encoder counts and arg2 retain their full 16-bit ranges.
   For obstacle events, arg2 carries depth 0..65535 mm, so it cannot be
   narrowed to the three bits sufficient for a barcode action.

   Only s1_log_push/pop access this storage through internal pack/unpack
   accessors. Invalid kind/arg values are rejected, never silently truncated.
   sizeof(s1_packed_event_t) is 10 bytes on the target: the existing storage
   was already 10 bytes, so this change does not claim savings per event. */
typedef struct
{
    uint16_t kind_arg;       /* bits 0..2 kind, 3..9 arg; bits 10..15 zero */
    uint16_t time_ms;
    uint16_t left_count;
    uint16_t right_count;
    uint16_t arg2;
} s1_packed_event_t;

/* Capacity 16: head 0..15 uses 4 bits; count 0..16 uses 5 bits.
   The combined 9 bits reside in one 16-bit storage unit. A push into a
   full ring overwrites the oldest event and saturates dropped; the newest
   events are retained. The ring is externally synchronised by its caller
   (subsystem1.c uses a bounded cross-core mutex). */
typedef struct
{
    s1_packed_event_t events[S1_EVENT_CAPACITY];
    uint16_t indices;       /* bits 0..3 head, 4..8 count; bits 9..15 zero */
    uint32_t dropped;
} s1_event_log_t;

_Static_assert(S1_EVENT_CAPACITY == 16u, "event-log index packing assumes 16 slots");
_Static_assert(sizeof(s1_packed_event_t) == 10u, "event packing must occupy 10 bytes");
_Static_assert(sizeof(s1_event_log_t) == 168u, "event ring layout changed unexpectedly");

void s1_log_init(s1_event_log_t *log);
bool s1_log_push(s1_event_log_t *log, const mqtt_event_t *event);
bool s1_log_pop(s1_event_log_t *log, mqtt_event_t *event); /* oldest first */
typedef bool (*s1_post_fn)(const uint8_t *frame, unsigned length);
/* Retains the oldest event if posting fails. Caller synchronizes the ring. */
bool s1_log_post_oldest(s1_event_log_t *log, s1_post_fn post);
unsigned s1_log_count(const s1_event_log_t *log);

/* Clamps a gathered telemetry record to its field widths in place, so the
   encoder never rejects it. */
void s1_clamp(mqtt_telemetry_t *t);

#endif /* S1_REPORT_H */
