/*
 *  trace_ring.h
 *  Index pair for a single-producer/single-consumer trace ring across cores.
 *  The caller owns the slot array (TRACE_RING_DEPTH entries). Full drops the
 *  newest frame; neither side waits. Each side does RMW only on its own index.
 */

#ifndef TRACE_RING_H
#define TRACE_RING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

enum { TRACE_RING_DEPTH = 8u }; /* frames; power of two so wrap stays exact */

typedef struct
{
    _Atomic uint32_t head; /* written only by the producer */
    _Atomic uint32_t tail; /* written only by the consumer */
} trace_ring_t;

/* Producer: free slot index, or false when full. Fill it, then commit. */
static inline bool trace_ring_reserve(trace_ring_t *ring, uint32_t *slot)
{
    uint32_t h = atomic_load_explicit(&ring->head, memory_order_relaxed);
    if ((h - atomic_load_explicit(&ring->tail, memory_order_acquire)) >=
        TRACE_RING_DEPTH)
    {
        return false;
    }
    *slot = h % TRACE_RING_DEPTH;
    return true;
}

/* Producer: publish the reserved slot to the consumer. */
static inline void trace_ring_commit(trace_ring_t *ring)
{
    uint32_t h = atomic_load_explicit(&ring->head, memory_order_relaxed);
    atomic_store_explicit(&ring->head, h + 1u, memory_order_release);
}

/* Consumer: oldest filled slot index, or false when empty. Copy, then
   release. */
static inline bool trace_ring_peek(trace_ring_t *ring, uint32_t *slot)
{
    uint32_t t = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    if (t == atomic_load_explicit(&ring->head, memory_order_acquire))
    {
        return false;
    }
    *slot = t % TRACE_RING_DEPTH;
    return true;
}

/* Consumer: hand the peeked slot back to the producer. */
static inline void trace_ring_release(trace_ring_t *ring)
{
    uint32_t t = atomic_load_explicit(&ring->tail, memory_order_relaxed);
    atomic_store_explicit(&ring->tail, t + 1u, memory_order_release);
}

#endif /* TRACE_RING_H */
