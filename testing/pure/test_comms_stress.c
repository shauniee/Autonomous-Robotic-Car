/* Host stress test (issue #43): telemetry inputs read while producers write.
   Pass criteria: every checked read is internally consistent (zero torn words,
   zero corrupted or reordered frames) over the stated number of reads. This
   models two cores with two threads; it does not replace an on-target run. */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/comms/comms.h"
#include "../../app_program/terrain/terrain.h"

#define MIN_WORD_READS 5000000u
#define FRAMES 300000u

static rd_word_t terrain_word;
static atomic_bool word_done;
static uint32_t frames_checked, frames_dropped, frames_bad;

/* The terrain writer publishes words whose fields are all derived from one
   counter; a torn read would show fields from different counters. */
static uint32_t make_word(uint32_t i)
{
    uint32_t count = i & 0xFFu, last = i & 0x7Eu, max = (i + 1u) & 0x7Eu;
    return terrain_pack(last, max, count, i & rd_mask(TERRAIN_STATE_BITS), (i & 1u) != 0u, true);
}

static void *terrain_writer(void *arg)
{
    (void)arg;
    for (uint32_t i = 0u; i < 3000000u; ++i) rd_word_publish(&terrain_word, make_word(i));
    atomic_store(&word_done, true);
    return NULL;
}

/* Outgoing ring: the producer (Reporting, core 1) posts, the consumer
   (transport, core 0) takes. Frames carry a counter and its inverse. */
static void *frame_producer(void *arg)
{
    (void)arg;
    uint8_t f[COMMS_FRAME_MAX];
    for (uint32_t i = 0u; i < FRAMES; ++i) {
        for (unsigned k = 0; k < 4u; ++k) {
            f[k] = (uint8_t)(i >> (8 * k));
            f[4 + k] = (uint8_t)~f[k];
        }
        memset(&f[8], (int)(i & 0xFFu), sizeof f - 8u);
        while (!comms_bridge_post(f, sizeof f)) { /* full: the consumer will catch up */ }
    }
    return NULL;
}

static void check_reconnect(void)
{
    uint8_t frame[] = {1u, 2u, 3u};
    uint8_t received[COMMS_FRAME_MAX];
    assert(!comms_bridge_post(frame, sizeof frame));
    comms_bridge_link(true, true);
    for (unsigned cycle = 0u; cycle < 25u; ++cycle)
    {
        for (unsigned i = 0u; i < COMMS_OUTGOING_DEPTH; ++i)
        {
            assert(comms_bridge_post(frame, sizeof frame));
        }
        assert(!comms_bridge_post(frame, sizeof frame));
        comms_bridge_put(frame, sizeof frame);
        comms_bridge_link(cycle % 2u == 0u, false);
        assert(!comms_bridge_ready());
        assert(comms_bridge_take(received, sizeof received) == 0u);
        assert(!comms_bridge_post(frame, sizeof frame));
        comms_bridge_put(frame, sizeof frame);
        comms_bridge_link(true, true);
        assert(comms_bridge_receive(received, sizeof received) == 0u);
        comms_bridge_put(frame, sizeof frame);
        comms_bridge_link(true, true);
        assert(comms_bridge_receive(received, sizeof received) == sizeof frame);
        assert(memcmp(frame, received, sizeof frame) == 0);
        assert(comms_bridge_take(received, sizeof received) == 0u);
        frame[0] = (uint8_t)cycle;
        assert(comms_bridge_post(frame, sizeof frame));
        /* An unchanged online report must preserve fresh queued data. */
        comms_bridge_link(true, true);
        assert(comms_bridge_take(received, sizeof received) == sizeof frame);
        assert(memcmp(frame, received, sizeof frame) == 0);
    }
    comms_bridge_put(frame, sizeof frame);
    comms_bridge_link(false, false);
    assert(comms_bridge_receive(received, sizeof received) == 0u);
    comms_bridge_link(true, true);
    assert(comms_bridge_receive(received, sizeof received) == 0u);
    comms_bridge_link(false, false);
}

int main(void)
{
    check_reconnect();
    assert(!comms_bridge_ready());
    comms_bridge_link(true, false);
    assert(!comms_bridge_ready());
    comms_bridge_link(true, true);
    assert(comms_bridge_ready());
    comms_bridge_link(false, false);
    assert(!comms_bridge_ready() && !rd_fault_stop_pending());
    comms_bridge_link(true, true);
    pthread_t tw, tp;
    uint32_t torn = 0u, reads = 0u;
    rd_word_publish(&terrain_word, make_word(0u));
    assert(pthread_create(&tw, NULL, terrain_writer, NULL) == 0);
    assert(pthread_create(&tp, NULL, frame_producer, NULL) == 0);

    uint8_t f[COMMS_FRAME_MAX];
    uint32_t next = 0u;
    while (reads < MIN_WORD_READS || !atomic_load(&word_done) || frames_checked < FRAMES) {
        uint32_t w = rd_word_read(&terrain_word);
        /* Reconstruct the counter from the count field and check every other field against it. */
        uint32_t i = terrain_hump_count(w);
        if (terrain_last_code(w) != (i & 0x7Eu) || terrain_max_code(w) != ((i + 1u) & 0x7Eu) ||
            terrain_state_code(w) != (i & rd_mask(TERRAIN_STATE_BITS)) || terrain_height_valid(w) != ((i & 1u) != 0u)) ++torn;
        ++reads;

        unsigned n = comms_bridge_take(f, sizeof f);
        if (n != 0u) {
            uint32_t seq = 0u;
            for (unsigned k = 0; k < 4u; ++k) seq |= (uint32_t)f[k] << (8 * k);
            bool ok = n == sizeof f;
            for (unsigned k = 0; k < 4u && ok; ++k) ok = f[4 + k] == (uint8_t)~f[k];
            for (unsigned k = 8; k < sizeof f && ok; ++k) ok = f[k] == (uint8_t)(seq & 0xFFu);
            if (!ok || seq != next) ++frames_bad;
            next = seq + 1u;
            ++frames_checked;
        }
        if (frames_checked >= FRAMES && atomic_load(&word_done) && reads >= MIN_WORD_READS) break;
    }
    pthread_join(tw, NULL);
    pthread_join(tp, NULL);
    printf("stress: %u word reads checked (%u torn), %u frames checked (%u corrupt or out of order)\n",
           reads, torn, frames_checked, frames_bad);
    assert(torn == 0u && frames_bad == 0u && reads >= MIN_WORD_READS && frames_checked == FRAMES);
    (void)frames_dropped;
    puts("comms_stress: PASS");
    return 0;
}
