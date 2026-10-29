#include "s1_bridge.h"
#include <stdatomic.h>
#include "../common/robot_data.h"

typedef struct
{
    uint8_t length;
    uint8_t bytes[S1_FRAME_MAX];
} slot_t;

typedef struct
{
    slot_t slots[S1_OUTGOING_DEPTH];
    _Atomic unsigned head, tail;	/* head: producer, tail: consumer */
} out_ring_t;

typedef struct
{
    slot_t slots[S1_INCOMING_DEPTH];
    _Atomic unsigned head, tail;
} in_ring_t;

static rd_word_t link_state; /* sole writer: kernel transport */

static out_ring_t outgoing;
static in_ring_t incoming;

static bool push(slot_t *slots, unsigned depth, _Atomic unsigned *head, _Atomic unsigned *tail,
                 const uint8_t *frame, unsigned length)
{
    if (frame == 0 || length == 0u || length > S1_FRAME_MAX) return false;
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

bool s1_bridge_post(const uint8_t *frame, unsigned length)
{
    return push(outgoing.slots, S1_OUTGOING_DEPTH, &outgoing.head, &outgoing.tail, frame, length);
}

unsigned s1_bridge_take(uint8_t *out, unsigned capacity)
{
    return pop(outgoing.slots, S1_OUTGOING_DEPTH, &outgoing.head, &outgoing.tail, out, capacity);
}

void s1_bridge_put(const uint8_t *frame, unsigned length)
{
    (void)push(incoming.slots, S1_INCOMING_DEPTH, &incoming.head, &incoming.tail, frame, length);
}

unsigned s1_bridge_receive(uint8_t *out, unsigned capacity)
{
    return pop(incoming.slots, S1_INCOMING_DEPTH, &incoming.head, &incoming.tail, out, capacity);
}

bool s1_bridge_ready(void)
{
    return rd_word_read(&link_state) == 3u;
}

void s1_bridge_link(bool wifi_up, bool broker_up)
{
    rd_word_publish(&link_state, (wifi_up ? 1u : 0u) | (broker_up ? 2u : 0u));
    /* The transport (core 0) is the only writer of the Network byte. */
    if (wifi_up) rd_fault_clear(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);
    else rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);
    if (broker_up) rd_fault_clear(RD_OWNER_NETWORK, RD_FAULT_BROKER_LOST);
    else rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_BROKER_LOST);
}
