/*
 *	comms_bridge.h
 *	The only application header the MQTT transport (lib/liblwip) includes.
 *	Frames cross as opaque bytes; the transport knows nothing of their layout
 *	(see mqtt_frame.h). Two fixed single-producer / single-consumer rings:
 *	  outgoing: I/O (core 0) posts, transport (core 0) takes
 *	  incoming: transport (core 0) puts, I/O (core 0) receives
 *	Wi-Fi or broker loss sets only the Network fault byte; it never stops motion.
 */

#ifndef COMMS_BRIDGE_H
#define COMMS_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#define COMMS_FRAME_MAX 23u		/* largest frame, MQTT_FRAME_MAX (telemetry/snapshot) */
#define COMMS_OUTGOING_DEPTH 4u
#define COMMS_INCOMING_DEPTH 2u

/* I/O side (core 0). post: false while offline or when the ring is full. */
bool comms_bridge_post(const uint8_t *frame, unsigned length);
/* Returns the length written to out, or 0 when nothing is waiting. */
unsigned comms_bridge_receive(uint8_t *out, unsigned capacity);

/* Transport side (core 0). take: length written, or 0 when nothing is waiting. */
unsigned comms_bridge_take(uint8_t *out, unsigned capacity);
/* Offline frames, oversized frames and frames arriving to a full ring drop. */
void comms_bridge_put(const uint8_t *frame, unsigned length);
/* Link state from transport; updates Network faults and discards queued TX/RX
   on loss/reconnection. Never changes motion or Mission state. */
void comms_bridge_link(bool wifi_up, bool broker_up);
/* Boot is disconnected. Mission uses this only as a local-start prerequisite. */
bool comms_bridge_ready(void);

#endif /* COMMS_BRIDGE_H */
