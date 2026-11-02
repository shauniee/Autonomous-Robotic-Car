#include "report.h"
#include "comms_bridge.h"
#include <stdatomic.h>
#include <stddef.h>
#include "../common/robot_data.h"

/* ---- Event log and telemetry clamping ---- */
/* Private accessors: no caller manipulates packed bits directly. */
enum
{
    COMMS_KIND_MASK = 0x07u,   /* 3-bit event kind */
    COMMS_ARG_MASK = 0x7Fu,    /* 7-bit event argument */
    COMMS_HEAD_MASK = 0x0Fu,  /* 4-bit ring head */
    COMMS_COUNT_MASK = 0x1Fu, /* 5-bit ring count */
    COMMS_ARG_SHIFT = 3u,
    COMMS_COUNT_SHIFT = 4u
};

static bool comms_event_pack(comms_packed_event_t *dst, const mqtt_event_t *src)
{
    if (dst == 0 || src == 0 || src->kind == 0u ||
        src->kind >= MQTT_EVENT_COUNT || src->arg > COMMS_ARG_MASK)
        return false;

    dst->kind_arg = (uint16_t)(((uint16_t)src->arg << COMMS_ARG_SHIFT) | src->kind);
    dst->time_ms = src->time_ms;
    dst->left_count = src->left_count;
    dst->right_count = src->right_count;
    dst->arg2 = src->arg2;
    return true;
}

static void comms_event_unpack(mqtt_event_t *dst, const comms_packed_event_t *src)
{
    *dst = (mqtt_event_t){
        .kind = (uint8_t)(src->kind_arg & COMMS_KIND_MASK),
        .arg = (uint8_t)((src->kind_arg >> COMMS_ARG_SHIFT) & COMMS_ARG_MASK),
        .time_ms = src->time_ms,
        .left_count = src->left_count,
        .right_count = src->right_count,
        .arg2 = src->arg2
    };
}

static unsigned comms_log_head(const comms_event_log_t *log)
{
    return (unsigned)(log->indices & COMMS_HEAD_MASK);
}

unsigned comms_log_count(const comms_event_log_t *log)
{
    return log == 0 ? 0u : (unsigned)((log->indices >> COMMS_COUNT_SHIFT) & COMMS_COUNT_MASK);
}

static void comms_log_set_indices(comms_event_log_t *log, unsigned head, unsigned count)
{
    log->indices = (uint16_t)((head & COMMS_HEAD_MASK) |
                              ((count & COMMS_COUNT_MASK) << COMMS_COUNT_SHIFT));
}

void comms_log_init(comms_event_log_t *log)
{
    if (log != 0) *log = (comms_event_log_t){0};
}

bool comms_log_push(comms_event_log_t *log, const mqtt_event_t *event)
{
    if (log == 0) return false;
    comms_packed_event_t packed;
    if (!comms_event_pack(&packed, event)) return false; /* no truncation */

    unsigned head = comms_log_head(log);
    unsigned count = comms_log_count(log);
    if (count == COMMS_EVENT_CAPACITY)
    {
        if (log->dropped != 0xFFFFFFFFu) ++log->dropped;
        /* head is also the oldest slot when full: overwrite it. */
    }
    else
    {
        ++count;
    }
    log->events[head] = packed;
    head = (head + 1u) % COMMS_EVENT_CAPACITY;
    comms_log_set_indices(log, head, count);
    return true;
}

bool comms_log_pop(comms_event_log_t *log, mqtt_event_t *event)
{
    if (log == 0 || event == 0) return false;
    unsigned count = comms_log_count(log);
    if (count == 0u) return false;
    unsigned oldest = (comms_log_head(log) + COMMS_EVENT_CAPACITY - count) % COMMS_EVENT_CAPACITY;
    comms_event_unpack(event, &log->events[oldest]);
    comms_log_set_indices(log, comms_log_head(log), count - 1u);
    return true;
}

bool comms_log_post_oldest(comms_event_log_t *log, comms_post_fn post)
{
    unsigned count = comms_log_count(log);
    if (count == 0u || post == NULL)
    {
        return false;
    }
    unsigned oldest = (comms_log_head(log) + COMMS_EVENT_CAPACITY - count) % COMMS_EVENT_CAPACITY;
    mqtt_event_t event;
    comms_event_unpack(&event, &log->events[oldest]);
    uint8_t frame[MQTT_FRAME_MAX];
    unsigned length = mqtt_encode_event(frame, sizeof frame, &event);
    if (length == 0u || !post(frame, length))
    {
        return false;
    }
    comms_log_set_indices(log, comms_log_head(log), count - 1u);
    return true;
}

static uint8_t clamp8(unsigned v, unsigned max) { return (uint8_t)(v > max ? max : v); }

void comms_clamp(mqtt_telemetry_t *t)
{
    t->mission_state = clamp8(t->mission_state, 15u);
    t->motion_status = clamp8(t->motion_status, 3u);
    t->motion_action = clamp8(t->motion_action, 63u);
    t->line_status = clamp8(t->line_status, 3u);
    t->hump_max = clamp8(t->hump_max, 127u);
    t->front_mm = (uint16_t)(t->front_mm > 8191u ? 8191u : t->front_mm);
    t->front_valid = t->front_valid && t->front_mm != 8191u;
    t->hump_valid = t->hump_valid && t->hump_max != 127u;
    t->barcode_letter = t->barcode_letter > 25u ? MQTT_NO_LETTER : t->barcode_letter;
    t->barcode_action = t->barcode_action > 4u ? 0u : t->barcode_action;
}

/* ---- Frame bridge to the MQTT transport ---- */
typedef struct
{
    uint8_t length;
    uint8_t bytes[COMMS_FRAME_MAX];
} slot_t;

typedef struct
{
    slot_t slots[COMMS_OUTGOING_DEPTH];
    _Atomic unsigned head, tail;	/* head: producer, tail: consumer */
} out_ring_t;

typedef struct
{
    slot_t slots[COMMS_INCOMING_DEPTH];
    _Atomic unsigned head, tail;
} in_ring_t;

static rd_word_t link_state; /* sole writer: kernel transport */

static out_ring_t outgoing;
static in_ring_t incoming;
static _Atomic unsigned incoming_floor; /* transport: first eligible RX slot */

static bool push(slot_t *slots, unsigned depth, _Atomic unsigned *head, _Atomic unsigned *tail,
                 const uint8_t *frame, unsigned length)
{
    if (frame == 0 || length == 0u || length > COMMS_FRAME_MAX) return false;
    unsigned h = atomic_load_explicit(head, memory_order_relaxed);
    if ((unsigned)(h - atomic_load_explicit(tail, memory_order_acquire)) >= depth) return false;
    slot_t *s = &slots[h % depth];
    for (unsigned i = 0u; i < length; ++i) s->bytes[i] = frame[i];
    s->length = (uint8_t)length;
    atomic_store_explicit(head, h + 1u, memory_order_release);
    return true;
}

static unsigned pop(slot_t *slots, unsigned depth, _Atomic unsigned *head, _Atomic unsigned *tail,
                    uint8_t *out, unsigned capacity)
{
    unsigned t = atomic_load_explicit(tail, memory_order_relaxed);
    if (t == atomic_load_explicit(head, memory_order_acquire)) return 0u;
    const slot_t *s = &slots[t % depth];
    if (out == 0 || capacity < s->length)
    {
        atomic_store_explicit(tail, t + 1u, memory_order_release);	/* cannot fit: drop it */
        return 0u;
    }
    for (unsigned i = 0u; i < s->length; ++i) out[i] = s->bytes[i];
    unsigned n = s->length;
    atomic_store_explicit(tail, t + 1u, memory_order_release);
    return n;
}

bool comms_bridge_post(const uint8_t *frame, unsigned length)
{
    if (!comms_bridge_ready())
    {
        return false;
    }
    return push(outgoing.slots, COMMS_OUTGOING_DEPTH, &outgoing.head, &outgoing.tail, frame, length);
}

unsigned comms_bridge_take(uint8_t *out, unsigned capacity)
{
    return pop(outgoing.slots, COMMS_OUTGOING_DEPTH, &outgoing.head, &outgoing.tail, out, capacity);
}

void comms_bridge_put(const uint8_t *frame, unsigned length)
{
    if (comms_bridge_ready())
    {
        (void)push(incoming.slots, COMMS_INCOMING_DEPTH, &incoming.head,
            &incoming.tail, frame, length);
    }
}

unsigned comms_bridge_receive(uint8_t *out, unsigned capacity)
{
    unsigned floor = atomic_load_explicit(&incoming_floor,
        memory_order_acquire);
    unsigned tail = atomic_load_explicit(&incoming.tail, memory_order_relaxed);
    if ((int32_t)(floor - tail) > 0)
    {
        /* Only the RX consumer advances tail, including connection-loss drops. */
        atomic_store_explicit(&incoming.tail, floor, memory_order_release);
    }
    unsigned length = pop(incoming.slots, COMMS_INCOMING_DEPTH, &incoming.head,
        &incoming.tail, out, capacity);
    return comms_bridge_ready() && floor == atomic_load_explicit(&incoming_floor,
        memory_order_acquire) ? length : 0u;
}

bool comms_bridge_ready(void)
{
    return rd_word_read(&link_state) == 3u;
}

void comms_bridge_link(bool wifi_up, bool broker_up)
{
    /* Only the transport consumes TX slots. Flush at loss and reconnection:
       a producer already posting at loss may publish after the first flush. */
    if (!wifi_up || !broker_up || !comms_bridge_ready())
    {
        unsigned head = atomic_load_explicit(&outgoing.head, memory_order_acquire);
        atomic_store_explicit(&outgoing.tail, head, memory_order_release);
        unsigned received = atomic_load_explicit(&incoming.head,
            memory_order_acquire);
        atomic_store_explicit(&incoming_floor, received, memory_order_release);
    }
    rd_word_publish(&link_state, (wifi_up ? 1u : 0u) | (broker_up ? 2u : 0u));
    /* The transport (core 0) is the only writer of the Network byte. */
    if (wifi_up) rd_fault_clear(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);
    else rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);
    if (broker_up) rd_fault_clear(RD_OWNER_NETWORK, RD_FAULT_BROKER_LOST);
    else rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_BROKER_LOST);
}
