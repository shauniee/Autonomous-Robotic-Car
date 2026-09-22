#ifndef ROBOT_COMMS_PROTOCOL_H
#define ROBOT_COMMS_PROTOCOL_H

#include <stddef.h>
#include "robot/types.h"

#define COMMS_COMMAND_MAX 192u         /* Maximum incoming command bytes; 'u' means unsigned. */
#define COMMS_SESSION_LENGTH 16u       /* Characters in a connection's random hex identifier. */
#define COMMS_COMMAND_MAX_AGE_MS 5000u /* Reject JSON commands older than 5 seconds. */
#define COMMS_TELEMETRY_MAX 2048u      /* Outgoing JSON buffer capacity, including final '\0'. */

typedef struct {
    char session[COMMS_SESSION_LENGTH + 1]; /* +1 stores the C string terminator, '\0'. */
    uint32_t last_id;                      /* Highest accepted ID; rejects repeated commands. */
    uint32_t pending_issued_ms;            /* Car uptime attached to the queued JSON command. */
    RobotCommand pending;                 /* NONE, START or STOP, waiting for the controller. */
    char payload[COMMS_COMMAND_MAX + 1];   /* Collects a message that arrives in fragments. */
    size_t expected;                       /* Total byte count announced by MQTT. */
    size_t received;                       /* Bytes copied into payload so far. */
    bool receiving;                       /* True while assembling an allowed message. */
} CommandInbox;

/* Start a new session and discard commands/fragments from the old one. */
void command_inbox_reset(CommandInbox *inbox, const char *session);
/* Called when MQTT announces a new publication and its total payload length. */
void command_inbox_begin(CommandInbox *inbox, size_t length, bool correct_topic);
/* Add a fragment. Only the last fragment triggers parsing of the full command. */
bool command_inbox_data(CommandInbox *inbox, const void *data, size_t length,
                        bool last, uint32_t now_ms);
/* Consume the pending command once, rechecking START's age before returning it. */
RobotCommand command_inbox_take(CommandInbox *inbox, uint32_t now_ms);
/* Queue a STOP, overriding any queued START. */
void command_inbox_stop(CommandInbox *inbox);

/* Returns bytes written, or zero on overflow. Output is always terminated
   when capacity is nonzero; never publish a buffer after an overflow. */
size_t comms_encode_telemetry(char *out, size_t capacity,
                             const VehicleController *controller,
                             const SensorFrame *sensors,
                             const MotionFeedback *motion, uint32_t now_ms);

#endif
