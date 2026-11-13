#include "line_trace.h"
#include <string.h>
#include "../common/trace_ring.h"

static uint8_t records[TRACE_RING_DEPTH][MQTT_LEN_LINE_TRACE];
static trace_ring_t ring;

bool line_trace_publish(const mqtt_line_trace_t *trace)
{
    uint32_t slot;
    if (!trace_ring_reserve(&ring, &slot) ||
        (mqtt_encode_line_trace(records[slot], MQTT_LEN_LINE_TRACE,
            trace) == 0u))
    {
        return false;
    }
    trace_ring_commit(&ring);
    return true;
}

unsigned line_trace_take(uint8_t *out, unsigned capacity)
{
    uint32_t slot;
    if (NULL == out || capacity < MQTT_LEN_LINE_TRACE ||
        !trace_ring_peek(&ring, &slot))
    {
        return 0u;
    }
    memcpy(out, records[slot], MQTT_LEN_LINE_TRACE);
    trace_ring_release(&ring);
    return MQTT_LEN_LINE_TRACE;
}
