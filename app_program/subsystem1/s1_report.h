/*
 *	s1_report.h
 *	Kernel-free Reporting logic: the event-log ring, telemetry assembly and
 *	the mapping-command flow. subsystem1.c supplies the sources and the
 *	transport; nothing here touches the kernel.
 */

#ifndef S1_REPORT_H
#define S1_REPORT_H

#include <stdbool.h>
#include <stdint.h>
#include "mqtt_frame.h"
#include "../common/robot_config.h"

/* ---- Event log: fixed ring of packed events ------------------------------
   Overflow is defined: a push into a full ring overwrites the oldest event
   and counts it in `dropped` (saturating); the newest events are always kept. */
typedef struct
{
    mqtt_event_t events[S1_EVENT_CAPACITY];
    unsigned head;		/* next write */
    unsigned count;		/* 0..S1_EVENT_CAPACITY */
    uint32_t dropped;
} s1_event_log_t;

void s1_log_init(s1_event_log_t *log);
void s1_log_push(s1_event_log_t *log, const mqtt_event_t *event);
bool s1_log_pop(s1_event_log_t *log, mqtt_event_t *event);	/* oldest first */

/* Clamps a gathered telemetry record to its field widths in place, so the
   encoder never rejects it. */
void s1_clamp(mqtt_telemetry_t *t);

/* ---- Mapping command -------------------------------------------------------
   install: called only when the robot is stopped; returns true when Mission
   installed the letters. Returns the acknowledgement to send, or false in
   *send when the frame is not a mapping at all. A valid mapping is acknowledged
   only after the install attempt, never on receipt alone. */
typedef bool (*s1_install_fn)(const uint8_t letters[4]);

typedef enum { S1_COMMAND_IGNORED, S1_COMMAND_MAPPING, S1_COMMAND_SNAPSHOT } s1_command_t;

s1_command_t s1_handle_command(const uint8_t *frame, unsigned length, bool robot_stopped,
                               s1_install_fn install, mqtt_ack_t *ack, bool *send_ack);

#endif /* S1_REPORT_H */
