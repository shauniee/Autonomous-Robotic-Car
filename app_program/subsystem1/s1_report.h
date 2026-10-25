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

/* ---- Telemetry sources: plain values read by subsystem1.c ----------------- */
typedef struct
{
    uint8_t mission_state;
    uint8_t motion_status, motion_action;
    uint8_t faults[MQTT_FAULT_BYTES];
    uint16_t left_count, right_count;
    uint8_t hump_count, hump_max_code;
    bool hump_valid;
    uint16_t front_mm;
    bool front_valid;
    uint8_t barcode_letter;	/* 0..25 or MQTT_NO_LETTER */
    uint8_t barcode_action;
} s1_sources_t;

/* Copies the sources into a telemetry record, clamping to the field widths. */
void s1_assemble(const s1_sources_t *sources, uint8_t sequence, mqtt_telemetry_t *out);

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
