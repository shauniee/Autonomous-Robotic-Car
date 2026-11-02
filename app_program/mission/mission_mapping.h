/* Single outstanding I/O -> Mission request, with acceptance-result reply. */
#ifndef MISSION_MAPPING_H
#define MISSION_MAPPING_H

#include "../comms/comms.h"

/* I/O is sole submit/poll caller. A full hand-off refuses another request. */
bool mission_mapping_submit(const mqtt_mapping_t *mapping);
bool mission_mapping_poll(mqtt_ack_t *ack);
/* I/O only; consecutive duplicate direction sequences never replay. */
bool mission_direction_submit(uint8_t sequence, uint8_t action);
/* Mission is sole take/reply caller; never install from an I/O callback. */
/* action 0 means mapping; 1..4 means junction test. Same single-slot hand-off. */
bool mission_mapping_take(mqtt_mapping_t *mapping, uint8_t *action);
void mission_mapping_reply(mqtt_ack_result_t result);

#endif /* MISSION_MAPPING_H */
