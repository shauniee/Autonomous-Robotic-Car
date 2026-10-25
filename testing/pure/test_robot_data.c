/* Host test for app_program/common/robot_data.[ch] (issue #27).
   Positive pass criterion for the stress test: STRESS_READS reads, zero torn. */
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/common/robot_data.h"

#define STRESS_READS 5000000u
#define STRESS_WRITES 2000000u

static rd_word_t shared;
static atomic_bool done;

/* Writer publishes words whose halves are equal; a torn read would differ. */
static void *writer(void *arg)
{
    (void)arg;
    for (uint32_t i = 0u; i < STRESS_WRITES; ++i) {
        uint32_t v = (i & 0xFFFFu);
        rd_word_publish(&shared, (v << 16) | v);
    }
    atomic_store(&done, true);
    return NULL;
}

static void test_fields(void)
{
    bool ok;
    uint32_t w = rd_field_set(0u, 3u, 5u, 31u, &ok);
    assert(ok && rd_field_get(w, 3u, 5u) == 31u && w == (31u << 3));
    uint32_t same = rd_field_set(w, 3u, 5u, 32u, &ok);  /* invalid: one past max */
    assert(!ok && same == w);
    w = rd_field_set(w, 3u, 5u, 0u, &ok);               /* minimum */
    assert(ok && w == 0u);
    assert(rd_sat_valid(0u, 7u) == 0u);
    assert(rd_sat_valid(126u, 7u) == 126u);
    assert(rd_sat_valid(127u, 7u) == 126u);             /* all-ones is invalid, never a value */
    assert(rd_sat_valid(100000u, 7u) == 126u);
    assert(rd_invalid_code(7u) == 127u);
}

static void test_mapping(void)
{
    uint32_t w = 99u;
    const uint8_t good[RD_MAP_COUNT] = {0u, 25u, 12u, 3u};         /* min, max, mid */
    assert(rd_mapping_pack(good, &w));
    assert(rd_mapping_is_mapped(w));
    assert(rd_mapping_letter(w, RD_MAP_LEFT) == 0u);
    assert(rd_mapping_letter(w, RD_MAP_RIGHT) == 25u);
    assert(rd_mapping_letter(w, RD_MAP_STRAIGHT) == 12u);
    assert(rd_mapping_letter(w, RD_MAP_UTURN) == 3u);
    assert((w >> 21) == 0u);                                        /* only 21 bits used */
    rd_map_action_t a;
    assert(rd_mapping_lookup(w, 25u, &a) && a == RD_MAP_RIGHT);
    assert(!rd_mapping_lookup(w, 7u, &a));                          /* letter not in mapping */
    assert(!rd_mapping_lookup(w, 26u, &a));                         /* invalid letter */
    assert(!rd_mapping_lookup(0u, 0u, &a));                         /* boot word: unmapped */

    uint32_t keep = w;
    const uint8_t dup[RD_MAP_COUNT] = {1u, 2u, 2u, 4u};
    const uint8_t big[RD_MAP_COUNT] = {1u, 2u, 3u, 26u};
    assert(!rd_mapping_pack(dup, &w) && w == keep);                 /* duplicate rejected */
    assert(!rd_mapping_pack(big, &w) && w == keep);                 /* out of A..Z rejected */
    assert(!rd_mapping_pack(good, NULL));
}

static void test_feedback(void)
{
    bool ok;
    uint32_t w = rd_motion_feedback_pack(RD_MOTION_FAILED, 63u, &ok);   /* max/max */
    assert(ok && rd_motion_feedback_status(w) == RD_MOTION_FAILED && rd_motion_feedback_action(w) == 63u);
    w = rd_motion_feedback_pack(RD_MOTION_IDLE, 0u, &ok);                /* min/min */
    assert(ok && w == 0u);
    w = rd_motion_feedback_pack(RD_MOTION_DONE, 64u, &ok);               /* action one past max */
    assert(!ok && w == 0u);
    w = rd_motion_feedback_pack((rd_motion_status_t)4, 1u, &ok);         /* invalid status code */
    assert(!ok && w == 0u);
}

static void test_faults(void)
{
    assert(rd_fault_aggregate() == 0u && !rd_fault_stop_pending());
    rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST | RD_FAULT_BROKER_LOST);
    assert(!rd_fault_stop_pending());                               /* network never stops motion */
    assert(rd_fault_aggregate() == 0u);
    for (unsigned o = 0u; o < RD_OWNER_NETWORK; ++o) {
        rd_fault_set((rd_owner_t)o, 0x81u);
        assert(rd_fault_stop_pending());
        assert(rd_fault_get((rd_owner_t)o) == 0x81u);
        assert(((rd_fault_aggregate() >> (8u * o)) & 0xFFu) == 0x81u);
        rd_fault_clear((rd_owner_t)o, 0x01u);                       /* clear only own listed bit */
        assert(rd_fault_get((rd_owner_t)o) == 0x80u);
        rd_fault_clear((rd_owner_t)o, 0x80u);
        assert(!rd_fault_stop_pending());
    }
    rd_fault_clear(RD_OWNER_NETWORK, 0xFFu);
    rd_fault_set((rd_owner_t)RD_OWNER_COUNT, 1u);                   /* invalid owner ignored */
    assert(rd_fault_get((rd_owner_t)RD_OWNER_COUNT) == 0u && rd_fault_aggregate() == 0u);
}

static void test_stress(void)
{
    pthread_t t;
    uint32_t torn = 0u, reads = 0u;
    rd_word_publish(&shared, 0u);
    assert(pthread_create(&t, NULL, writer, NULL) == 0);
    while (reads < STRESS_READS || !atomic_load(&done)) {
        uint32_t v = rd_word_read(&shared);
        if ((v >> 16) != (v & 0xFFFFu)) {
            ++torn;
        }
        ++reads;
    }
    pthread_join(t, NULL);
    printf("stress: %u reads checked, %u torn\n", reads, torn);
    assert(torn == 0u && reads >= STRESS_READS);
}

int main(void)
{
    test_fields();
    test_mapping();
    test_feedback();
    test_faults();
    test_stress();
    puts("robot_data: PASS");
    return 0;
}
