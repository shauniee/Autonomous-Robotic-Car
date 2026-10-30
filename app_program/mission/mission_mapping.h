/* Single outstanding I/O -> Mission mapping, with installed-result reply. */
#ifndef MISSION_MAPPING_H
#define MISSION_MAPPING_H

#include "../subsystem1/mqtt_frame.h"

/* I/O is sole submit/poll caller. A full hand-off refuses another request. */
bool mission_mapping_submit(const mqtt_mapping_t *mapping);
bool mission_mapping_poll(mqtt_ack_t *ack);
/* Mission is sole take/reply caller; never install from an I/O callback. */
bool mission_mapping_take(mqtt_mapping_t *mapping);
void mission_mapping_reply(mqtt_ack_result_t result);

#endif /* MISSION_MAPPING_H */
