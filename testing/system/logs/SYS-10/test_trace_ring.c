#include <assert.h>
#include <stdio.h>
#include "../../../../app_program/common/trace_ring.h"

int main(void)
{
    trace_ring_t ring = {0};
    uint32_t slot = 99u;

    /* Empty: nothing to peek. */
    assert(!trace_ring_peek(&ring, &slot) && slot == 99u);

    /* Fill to depth, then the next reserve is refused (drop newest). */
    for (uint32_t i = 0u; i < TRACE_RING_DEPTH; ++i)
    {
        assert(trace_ring_reserve(&ring, &slot) && slot == i);
        trace_ring_commit(&ring);
    }
    assert(!trace_ring_reserve(&ring, &slot));

    /* Oldest first; one release frees one slot. */
    assert(trace_ring_peek(&ring, &slot) && slot == 0u);
    trace_ring_release(&ring);
    assert(trace_ring_reserve(&ring, &slot) && slot == 0u);

    /* Index wrap at 2^32 keeps count and slot exact. */
    atomic_store(&ring.head, UINT32_MAX);
    atomic_store(&ring.tail, UINT32_MAX);
    assert(trace_ring_reserve(&ring, &slot) && slot == (UINT32_MAX % TRACE_RING_DEPTH));
    trace_ring_commit(&ring);
    assert(trace_ring_reserve(&ring, &slot) && slot == 0u);
    trace_ring_commit(&ring);
    assert(trace_ring_peek(&ring, &slot) && slot == (UINT32_MAX % TRACE_RING_DEPTH));
    trace_ring_release(&ring);
    assert(trace_ring_peek(&ring, &slot) && slot == 0u);
    trace_ring_release(&ring);
    assert(!trace_ring_peek(&ring, &slot));

    puts("trace_ring: ok");
    return 0;
}
