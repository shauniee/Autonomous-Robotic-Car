/* Host test for the hump estimator and terrain word (issue #46). */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/subsystem4/terrain_algo.h"
#include "../../app_program/subsystem4/terrain_pack.h"

static TerrainContext ctx;

static void calibrate(void)
{
    terrain_algo_init(&ctx);
    Gy511RawAxes flat = {0, 0, (int16_t)lroundf(1.0f / ACCEL_MG_PER_LSB)};
    for (int i = 0; i < 100; ++i) terrain_algo_calibrate_sample(&ctx, &flat);
    assert(ctx.is_calibrated);
}

/* Feed `n` samples at a pitch (degrees), each with `counts` of forward travel. */
static void feed(float pitch_deg, int n, uint32_t counts)
{
    float r = pitch_deg * 0.0174532925f;
    Gy511RawAxes a = {0, (int16_t)lroundf(sinf(r) / ACCEL_MG_PER_LSB), (int16_t)lroundf(cosf(r) / ACCEL_MG_PER_LSB)};
    for (int i = 0; i < n; ++i) terrain_algo_process(&ctx, &a, counts);
}

static void test_complete_hump(void)
{
    calibrate();
    feed(0.0f, 30, 0u);
    feed(10.0f, 40, 4u);			/* climb: about 4 counts = 2.3 mm per step */
    feed(0.0f, 4, 4u);				/* crest: pitch passes through zero briefly */
    feed(-10.0f, 40, 4u);			/* descent */
    feed(0.0f, 30, 0u);				/* flat again */
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.height_valid && ctx.hump_count == 1u);
    uint32_t mm;
    assert(terrain_code_to_mm(ctx.last_hump_code, &mm) && mm >= 8u && mm <= 40u);
    assert(ctx.max_hump_code == ctx.last_hump_code);

    /* A second, smaller hump raises the count but not the maximum. */
    uint8_t first = ctx.last_hump_code;
    feed(10.0f, 25, 4u); feed(0.0f, 4, 4u); feed(-10.0f, 40, 4u); feed(0.0f, 30, 0u);
    assert(ctx.hump_count == 2u && ctx.max_hump_code == first && ctx.last_hump_code < first);
}

static void test_incomplete_and_small_encounters_add_nothing(void)
{
    calibrate();
    feed(10.0f, 40, 4u);				/* climb ... */
    feed(0.0f, 60, 0u);				/* ... then flatten with no descent */
    assert(ctx.hump_count == 0u && !ctx.height_valid && ctx.state == TERRAIN_STATE_FLAT);

    calibrate();					/* a bump below the minimum height */
    feed(5.0f, 6, 1u); feed(0.0f, 30, 1u); feed(-5.0f, 20, 1u); feed(0.0f, 30, 0u);
    assert(ctx.hump_count == 0u && ctx.last_hump_code == 0u);

    calibrate();					/* stationary climb: no travel, no height */
    feed(10.0f, 30, 0u); feed(0.0f, 30, 0u); feed(-10.0f, 30, 0u); feed(0.0f, 30, 0u);
    assert(ctx.hump_count == 0u);
}

static void test_saturation_and_count(void)
{
    calibrate();
    feed(10.0f, 60, 400u);				/* enormous rise saturates, never reaches the invalid code */
    feed(0.0f, 4, 4u); feed(-10.0f, 60, 4u); feed(0.0f, 30, 0u);
    assert(ctx.hump_count == 1u && ctx.last_hump_code == rd_invalid_code(HUMP_RESULT_BITS) - 1u);
    for (unsigned i = 0; i < 11u; ++i) {
        feed(10.0f, 40, 4u); feed(0.0f, 4, 4u); feed(-10.0f, 40, 4u); feed(0.0f, 30, 0u);
    }
    assert(ctx.hump_count == 12u);
}

static void test_word_boundaries(void)
{
    const uint32_t inv = rd_invalid_code(HUMP_RESULT_BITS);
    uint32_t w = terrain_pack(0u, 0u, 0u, 0u, false, false);
    assert(w == 0u && !terrain_height_valid(w) && !terrain_calibrated(w));
    w = terrain_pack(inv - 1u, inv - 1u, 255u, 4u, true, true);			/* maxima */
    assert(terrain_last_code(w) == inv - 1u && terrain_max_code(w) == inv - 1u && terrain_hump_count(w) == 255u);
    assert(terrain_state_code(w) == 4u && terrain_height_valid(w) && terrain_calibrated(w));
    w = terrain_pack(1000u, 1000u, 1000u, 15u, false, true);			/* beyond range: clamps, never spills */
    assert(terrain_last_code(w) == inv && terrain_max_code(w) == inv && terrain_hump_count(w) == 255u);
    assert(terrain_state_code(w) == 7u && (w >> 27) == 0u);
    uint32_t mm;
    assert(!terrain_code_to_mm(inv, &mm));						/* invalid code has no height */
    assert(terrain_code_to_mm(0u, &mm) && mm == 0u);
    assert(terrain_code_to_mm(inv - 1u, &mm) && mm == (inv - 1u) * HUMP_MM_PER_COUNT);
}

int main(void)
{
    test_complete_hump();
    test_incomplete_and_small_encounters_add_nothing();
    test_saturation_and_count();
    test_word_boundaries();
    puts("terrain: PASS");
    return 0;
}
