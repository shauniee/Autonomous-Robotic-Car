/*
 *	s1_bridge.h
 *	The only application header the MQTT transport (lib/liblwip) includes.
 *	Frames cross as opaque bytes; the transport knows nothing of their layout
 *	(see mqtt_frame.h). Two fixed single-producer / single-consumer rings:
 *	  outgoing: Reporting (core 1) posts, transport (core 0) takes
 *	  incoming: transport (core 0) puts, Reporting (core 1) receives
 *	Wi-Fi or broker loss sets only the Network fault byte; it never stops motion.
 */

#ifndef S1_BRIDGE_H
#define S1_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#define S1_FRAME_MAX 23u		/* largest frame, MQTT_FRAME_MAX (telemetry/snapshot) */
#define S1_OUTGOING_DEPTH 4u
#define S1_INCOMING_DEPTH 2u

/* Reporting side (core 1). post: false when the ring is full (frame dropped). */
bool s1_bridge_post(const uint8_t *frame, unsigned length);
/* Returns the length written to out, or 0 when nothing is waiting. */
unsigned s1_bridge_receive(uint8_t *out, unsigned capacity);

/* Transport side (core 0). take: length written, or 0 when nothing is waiting. */
unsigned s1_bridge_take(uint8_t *out, unsigned capacity);
/* Frames longer than S1_FRAME_MAX or arriving to a full ring are dropped. */
void s1_bridge_put(const uint8_t *frame, unsigned length);
/* Link state from the transport; sets the Network fault byte only. */
void s1_bridge_link(bool wifi_up, bool broker_up);

#endif /* S1_BRIDGE_H */
