/* Host test for app_program/motion/encoder_math.h (issue #29). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/motion/encoder_math.h"

int main(void)
{
    /* INT-66: independently recorded wheel deltas over the measured 500 mm. */
    const uint32_t measured_before[2] = {2210u, 9u};
    const uint32_t measured_after[2] = {8043u, 5825u};
    float measured_mm = enc_mean_travel(measured_after, measured_before) /
        ENC_COUNTS_PER_MM;
    assert(measured_mm > 499.99f && measured_mm < 500.01f);
    float left_mm = 5833.0f / ENC_COUNTS_PER_MM;
    float right_mm = 5816.0f / ENC_COUNTS_PER_MM;
    assert(left_mm > 499.0f && left_mm < 501.0f);
    assert(right_mm > 499.0f && right_mm < 501.0f);
    /* Current tracking trial and recovery speeds in measured encoder units. */
    float follow_cps = MISSION_TRACK_SPEED_MMPS * ENC_COUNTS_PER_MM;
    float recovery_cps = RECOVERY_SPEED_MMPS * ENC_COUNTS_PER_MM;
    assert(follow_cps > 1164.0f && follow_cps < 1166.0f); /* 100 mm/s, INT-123 */
    assert(recovery_cps > 410.0f && recovery_cps < 435.0f);
    /* Lifetime wheel separation must not affect local displacement. */
    const uint32_t before[2] = {1000u, 33767u};
    const uint32_t after[2] = {1001u, 33769u};
    assert(enc_mean_travel(after, before) == 1.5f);
    assert(enc_mean_travel(before, after) == -1.5f);
    const uint32_t wrapping[2] = {ENC_COUNT_MASK, 1234u};
    const uint32_t advanced[2] = {1u, 1236u};
    assert(enc_mean_travel(advanced, wrapping) == 2.0f);
    const uint32_t pivot[2] = {999u, 33768u};
    assert(enc_mean_travel(pivot, before) == 0.0f);
    assert(enc_mean_travel(before, before) == 0.0f);
    /* Quadrature: forward Gray cycle 00,01,11,10 gives +1 each step, reverse -1. */
    const unsigned fwd[] = {0u, 1u, 3u, 2u};
    for (unsigned i = 0u; i < 4u; ++i) {
        unsigned a = fwd[i], b = fwd[(i + 1u) % 4u];
        assert(enc_quad_step(a, b) == 1);
        assert(enc_quad_step(b, a) == -1);
        assert(enc_quad_step(a, a) == 0);
        assert(enc_quad_step(a, fwd[(i + 2u) % 4u]) == ENC_QUAD_INVALID);   /* skipped state */
    }
    assert(enc_quad_step(4u, 0u) == ENC_QUAD_INVALID && enc_quad_step(0u, 4u) == ENC_QUAD_INVALID);

    /* Travel across the wrap at the configured width. */
    uint32_t origin = ENC_COUNT_MASK - 2u;
    uint32_t count = origin;
    for (int i = 0; i < 10; ++i) {
        count = enc_advance(count, 1);
    }
    assert(count == 7u);                                    /* wrapped */
    assert(enc_travel(count, origin) == 10u);
    assert(enc_travel_signed(count, origin) == 10);

    /* Reverse across zero. */
    count = enc_advance(0u, -3);
    assert(count == ENC_COUNT_MASK - 2u);
    assert(enc_travel_signed(count, 0u) == -3);
    assert(enc_travel(count, 0u) == ENC_COUNT_MASK - 2u);

    /* Two consumers with different origins stay independent while the shared
       count keeps running and is never reset. */
    uint32_t shared = ENC_COUNT_MASK - 5u;
    uint32_t origin_a = shared;
    for (int i = 0; i < 4; ++i) shared = enc_advance(shared, 1);
    uint32_t origin_b = shared;                              /* b starts 4 counts later */
    for (int i = 0; i < 20; ++i) shared = enc_advance(shared, 1);   /* crosses wrap */
    assert(enc_travel(shared, origin_a) == 24u);
    assert(enc_travel(shared, origin_b) == 20u);
    /* b's measurement is unaffected by a re-reading its own origin. */
    origin_a = shared;
    assert(enc_travel(shared, origin_a) == 0u && enc_travel(shared, origin_b) == 20u);

    /* Longest unambiguous forward travel is the mask; one more count aliases to 0. */
    assert(enc_travel(enc_advance(origin_b, 1), origin_b) == 1u);
    assert(enc_travel(origin_b, origin_b) == 0u);

    puts("encoder_math: PASS");
    puts("INT-82: measured rolling scale and tracking/recovery count-speed targets PASS");
    return 0;
}
