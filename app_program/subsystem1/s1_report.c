#include "s1_report.h"

void s1_log_init(s1_event_log_t *log)
{
    *log = (s1_event_log_t){0};
}

void s1_log_push(s1_event_log_t *log, const mqtt_event_t *event)
{
    if (log->count == S1_EVENT_CAPACITY)
    {
        if (log->dropped != 0xFFFFFFFFu) ++log->dropped;
        /* head is also the oldest slot when full: overwrite it. */
    }
    else
    {
        ++log->count;
    }
    log->events[log->head] = *event;
    log->head = (log->head + 1u) % S1_EVENT_CAPACITY;
}

bool s1_log_pop(s1_event_log_t *log, mqtt_event_t *event)
{
    if (log->count == 0u) return false;
    unsigned oldest = (log->head + S1_EVENT_CAPACITY - log->count) % S1_EVENT_CAPACITY;
    *event = log->events[oldest];
    --log->count;
    return true;
}

static uint8_t clamp8(unsigned v, unsigned max) { return (uint8_t)(v > max ? max : v); }

void s1_clamp(mqtt_telemetry_t *t)
{
    t->mission_state = clamp8(t->mission_state, 15u);
    t->motion_status = clamp8(t->motion_status, 3u);
    t->motion_action = clamp8(t->motion_action, 63u);
    t->line_status = clamp8(t->line_status, 3u);
    t->hump_max = clamp8(t->hump_max, 127u);
    t->front_mm = (uint16_t)(t->front_mm > 8191u ? 8191u : t->front_mm);
    t->barcode_letter = t->barcode_letter > 25u ? MQTT_NO_LETTER : t->barcode_letter;
    t->barcode_action = clamp8(t->barcode_action, 7u);
}

s1_command_t s1_handle_command(const uint8_t *frame, unsigned length, bool robot_stopped,
                               s1_install_fn install, mqtt_ack_t *ack, bool *send_ack)
{
    *send_ack = false;
    mqtt_type_t type;
    if (mqtt_frame_type(frame, length, &type) != MQTT_DECODE_OK)
    {
        return S1_COMMAND_IGNORED;	/* wrong version, length or type: no reply to guess at */
    }
    if (type == MQTT_TYPE_SNAPSHOT_REQUEST)
    {
        uint8_t sequence;
        return mqtt_decode_snapshot_request(frame, length, &sequence) == MQTT_DECODE_OK
            ? S1_COMMAND_SNAPSHOT : S1_COMMAND_IGNORED;
    }
    if (type != MQTT_TYPE_MAPPING)
    {
        return S1_COMMAND_IGNORED;	/* robot-to-PC types and acknowledgements are not commands */
    }
    mqtt_mapping_t mapping;
    mqtt_decode_t decoded = mqtt_decode_mapping(frame, length, &mapping);
    *send_ack = true;
    if (decoded != MQTT_DECODE_OK)
    {
        ack->sequence = frame[1];	/* the header and length were valid, so the sequence byte exists */
        ack->result = decoded == MQTT_DECODE_RANGE ? MQTT_ACK_INVALID : MQTT_ACK_MALFORMED;
    }
    else if (!robot_stopped)
    {
        ack->sequence = mapping.sequence;
        ack->result = MQTT_ACK_BUSY;	/* mapping while moving is refused */
    }
    else
    {
        ack->sequence = mapping.sequence;
        ack->result = (install != 0 && install(mapping.letters)) ? MQTT_ACK_OK : MQTT_ACK_INVALID;
    }
    return S1_COMMAND_MAPPING;
}
