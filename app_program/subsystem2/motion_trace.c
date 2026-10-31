#include "motion_trace.h"
#include <stdatomic.h>
#include <string.h>

enum { TRACE_DEPTH = 8u };
static uint8_t records[TRACE_DEPTH][MQTT_LEN_WHEEL_TRACE];
static _Atomic uint32_t head, tail;

bool motion_trace_publish(const mqtt_wheel_trace_t *trace)
{
    uint32_t h = atomic_load_explicit(&head, memory_order_relaxed);
    if (h - atomic_load_explicit(&tail, memory_order_acquire) >= TRACE_DEPTH)
    {
        return false;
    }
    if (mqtt_encode_wheel_trace(records[h % TRACE_DEPTH],
            MQTT_LEN_WHEEL_TRACE, trace) == 0u)
    {
        return false;
    }
    atomic_store_explicit(&head, h + 1u, memory_order_release);
    return true;
}

unsigned motion_trace_take(uint8_t *out, unsigned capacity)
{
    uint32_t t = atomic_load_explicit(&tail, memory_order_relaxed);
    if (out == NULL || capacity < MQTT_LEN_WHEEL_TRACE ||
        t == atomic_load_explicit(&head, memory_order_acquire))
    {
        return 0u;
    }
    memcpy(out, records[t % TRACE_DEPTH], MQTT_LEN_WHEEL_TRACE);
    atomic_store_explicit(&tail, t + 1u, memory_order_release);
    return MQTT_LEN_WHEEL_TRACE;
}
