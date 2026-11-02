/* Shaun: communications. Reporting task: binary telemetry, event log, mapping command. */
#ifndef COMMS_H
#define COMMS_H
#include "../app.h"
#include "mqtt_frame.h"
ER comms_init(void);
void comms_step(uint32_t now_ms);

/* Log one event with the current time and encoder position. Mission (core 1) or Terrain (core 0);
   insertion waits at most RD_SNAPSHOT_LOCK_MS (2 ms); failed locks are
   counted in per-core saturating lock_misses words; a full ring overwrites the oldest event (counted, see report.h).
   kind: mqtt_event_kind_t; arg and arg2 as documented in MQTT_PROTOCOL.md. */
void comms_event(mqtt_event_kind_t kind, uint8_t arg, uint16_t arg2);
#endif
