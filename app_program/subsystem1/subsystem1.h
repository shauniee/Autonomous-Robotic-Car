/* Shaun: communications. Reporting task: binary telemetry, event log, mapping command. */
#ifndef SUBSYSTEM1_H
#define SUBSYSTEM1_H
#include "../app.h"
#include "mqtt_frame.h"
ER subsystem1_init(void);
void s1_step(uint32_t now_ms);

/* Log one event with the current time and encoder position. Mission (core 1) or Terrain (core 0);
   insertion waits at most RD_SNAPSHOT_LOCK_MS (2 ms); failed locks are
   counted in per-core saturating lock_misses words; a full ring overwrites the oldest event (counted, see s1_report.h).
   kind: mqtt_event_kind_t; arg and arg2 as documented in MQTT_PROTOCOL.md. */
void s1_event(mqtt_event_kind_t kind, uint8_t arg, uint16_t arg2);
#endif
