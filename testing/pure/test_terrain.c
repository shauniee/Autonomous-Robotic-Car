#include "../../app_program/terrain/terrain.h"
/* Host test for the hump estimator and terrain word (issue #46). */
#include <assert.h>
#include <math.h>
#include <stdio.h>

static TerrainContext ctx;

static void calibrate(void)
{
    terrain_algo_init(&ctx);
    Gy511RawAxes flat = {0, 0, (int16_t)lroundf(1.0f / ACCEL_MG_PER_LSB)};
    for (int i = 0; i < 100; ++i) {
        terrain_algo_calibrate_sample(&ctx, &flat);
    }
    assert(ctx.is_calibrated);
}

static Gy511RawAxes tilted(float roll_deg, float pitch_deg)
{
    float r = roll_deg * 0.0174532925f, q = pitch_deg * 0.0174532925f;
    return (Gy511RawAxes){(int16_t)lroundf(sinf(r) / ACCEL_MG_PER_LSB),
        (int16_t)lroundf(sinf(q) / ACCEL_MG_PER_LSB),
        (int16_t)lroundf(cosf(r) * cosf(q) / ACCEL_MG_PER_LSB)};
}

/* Feed a physical travel step so encoder calibration does not shrink fixtures. */
static void feed(float roll_deg, int n, float step_mm)
{
    Gy511RawAxes a = tilted(roll_deg, 0.0f);
    for (int i = 0; i < n; ++i) {
        terrain_algo_process(&ctx, &a, step_mm * ENC_COUNTS_PER_MM, step_mm <= 0.0f);
    }
}

static void test_complete_hump(void)
{
    calibrate();
    feed(0.0f, 30, 2.0f);
    feed(-7.0f, 50, 2.0f);
    assert(ctx.state == TERRAIN_STATE_ON_HUMP && ctx.hump_count == 0u);
    feed(0.0f, 30, 2.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.height_valid && ctx.hump_count == 1u);
    uint32_t mm;
    assert(terrain_code_to_mm(ctx.last_hump_code, &mm) && mm >= 11u && mm <= 12u); /* 120 mm track, 7 degrees, calibration */
    assert(ctx.max_hump_code == ctx.last_hump_code);

    /* A second, smaller hump, tilting the other way, raises the count but not the maximum. */
    uint8_t first = ctx.last_hump_code;
    feed(5.0f, 50, 2.0f); feed(0.0f, 30, 2.0f);
    assert(ctx.hump_count == 2u && ctx.max_hump_code == first && ctx.last_hump_code < first);
}

/* Replays traced roll, in quarter degrees at about 2.4 mm a row. The trace is the
   filter's output and truncates toward zero, so take the middle of each value and
   feed the input that makes the filter here reproduce it. */
static void replay(const int8_t *roll, unsigned count)
{
    float previous = 0.0f;
    for (unsigned i = 0u; i < count; ++i) {
        int q = roll[i];
        float deg = (q < 0 ? q - 0.5f : q > 0 ? q + 0.5f : 0.0f) / 4.0f;
        feed((deg - 0.8f * previous) / 0.2f, 1, 2.4f);
        previous = deg;
    }
}

/* INT-155 run. */
static void test_real_crossing(void)
{
    static const int8_t hump[] = {-2, 4, 3, 2, 3, 4, 3, 4, 0, -2, -4, -6, -6, -8, -8, -5, -4, -6, -7, -8,
        -12, -17, -15, -13, -11, -8, -9, -8, -8, -11, -8, -8, -11, -15, -16, -17, -16, -17, -17, -21,
        -24, -26, -22, -23, -27, -24, -25, -23, -18, -20, -22, -24, -25, -28, -28, -21, -16, -11, -6, -7,
        -4, -2, -5, -8, -9, -6, -4, -7, -8, -10, -11, -11, -9, -8, -5, -2, -7, -6, -5, -3, -1, -3, -3,
        -4, -4, -2, -2, -3, -3};
    static const int8_t not_hump[] = {-3, -2, 0, -1, -1, 1, 7, 12, 10, 5, 0, -2, -3, -5, -10, -6, 0, 3, 7, 4, 6, 6};
    calibrate();
    replay(not_hump, sizeof not_hump);
    feed(0.0f, 20, 2.0f);
    assert(ctx.hump_count == 0u);
    replay(hump, sizeof hump);
    feed(0.0f, 20, 2.0f);
    assert(ctx.hump_count == 1u && ctx.height_valid);
}

static void test_a_dip_does_not_split_a_hump(void)
{
    calibrate();
    feed(-7.0f, 30, 2.0f); feed(0.0f, 8, 2.0f); feed(-7.0f, 30, 2.0f); feed(0.0f, 30, 2.0f);
    assert(ctx.hump_count == 1u);
}

static void test_what_is_not_a_hump(void)
{
    calibrate();					/* short tilt: not enough travel */
    feed(-7.0f, 4, 2.0f); feed(0.0f, 30, 0.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u && !ctx.height_valid);

    calibrate();					/* tilted standing still */
    feed(-7.0f, 60, 0.0f); feed(0.0f, 30, 0.0f);
    assert(ctx.hump_count == 0u);

    calibrate();					/* below the entry level, as on a curve */
    feed(3.0f, 200, 2.0f); feed(0.0f, 30, 2.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);

    calibrate();					/* lifted by hand while the wheels run */
    feed(30.0f, 60, 2.0f); feed(0.0f, 30, 2.0f);
    assert(ctx.hump_count == 0u && !ctx.height_valid && ctx.last_hump_code == 0u);

    calibrate();					/* pitch from speed changes or a two-wheel ramp */
    Gy511RawAxes nose_up = tilted(0.0f, 12.0f), nose_down = tilted(0.0f, -12.0f);
    for (int i = 0; i < 60; ++i) {
        terrain_algo_process(&ctx, &nose_up, 23.0f, false);
    }
    for (int i = 0; i < 60; ++i) {
        terrain_algo_process(&ctx, &nose_down, 23.0f, false);
    }
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);
}

/* INT-157: one boot pose read 6.5 degrees pitch and -1 degree roll on flat ground,
   and curves then reached the entry level. Offsets must not count, or stick. */
static void test_baseline(void)
{
    calibrate();
    feed(-2.0f, 300, 0.0f);				/* settles on the offset standing still */
    feed(-4.5f, 25, 2.0f); feed(-2.0f, 30, 2.0f);	/* a curve: 2.5 degrees from it */
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);
    feed(-7.5f, 50, 2.0f); feed(-2.0f, 30, 2.0f);	/* a hump, measured from the offset */
    assert(ctx.hump_count == 1u);
    uint32_t mm;
    assert(terrain_code_to_mm(ctx.last_hump_code, &mm) && mm >= 9u && mm <= 10u);

    calibrate();					/* tilted at rest and driving off is not a hump */
    feed(-7.0f, 100, 0.0f); feed(-7.0f, 50, 2.0f); feed(-7.0f, 30, 2.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);

    calibrate();					/* INT-158: a pivot has no mean travel, and is not stopped */
    Gy511RawAxes pivot = tilted(2.5f, 0.0f);
    for (int i = 0; i < 100; ++i) {
        terrain_algo_process(&ctx, &pivot, 0.0f, false);
    }
    feed(0.0f, 30, 2.0f);
    assert(ctx.roll_base_q8 == 0 && ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);

    calibrate();					/* a long tilt becomes the new ground */
    feed(0.0f, 30, 2.0f);
    feed(-7.0f, 300, 2.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);
    feed(-13.0f, 50, 2.0f); feed(-7.0f, 30, 2.0f);	/* and humps still count on it */
    assert(ctx.hump_count == 1u);

    calibrate();					/* a short curve-sized tilt past entry */
    feed(0.0f, 30, 2.0f);
    feed(-5.0f, 12, 2.0f); feed(0.0f, 30, 2.0f);
    assert(ctx.state == TERRAIN_STATE_FLAT && ctx.hump_count == 0u);
}

static void test_count(void)
{
    calibrate();
    for (unsigned i = 0; i < 12u; ++i) {
        feed(-7.0f, 50, 2.0f); feed(0.0f, 30, 2.0f);
    }
    assert(ctx.hump_count == 12u);
}

static void test_impact_and_restart(void)
{
    calibrate();
    Gy511RawAxes shock = {(int16_t)lroundf(3.0f / ACCEL_MG_PER_LSB), 0,
                          (int16_t)lroundf(1.0f / ACCEL_MG_PER_LSB)};
    terrain_algo_process(&ctx, &shock, 0.0f, true);
    assert(ctx.state == TERRAIN_STATE_IMPACT);
    Gy511RawAxes flat = {0, 0, (int16_t)lroundf(1.0f / ACCEL_MG_PER_LSB)};
    terrain_algo_process(&ctx, &flat, 0.0f, true);
    assert(ctx.state != TERRAIN_STATE_IMPACT);

    terrain_algo_init(&ctx);
    for (unsigned i = 0u; i < 37u; ++i) {
        terrain_algo_calibrate_sample(&ctx, &flat);
    }
    terrain_algo_calibration_restart(&ctx);
    assert(ctx.calib_count == 0u);
    for (unsigned i = 0u; i < 100u; ++i) {
        terrain_algo_calibrate_sample(&ctx, &flat);
    }
    assert(ctx.is_calibrated);
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

static void test_evidence_and_invalid_window(void)
{
    enc_count_t origin[2] = {1000u, 33767u};
    const uint32_t pending[2] = {1010u, 33777u};
    const uint32_t fresh[2] = {1020u, 33788u};
    assert(terrain_travel_take(origin, pending, false, false) == 0.0f);
    assert(origin[0] == 1000u && origin[1] == 33767u);
    assert(terrain_travel_take(origin, fresh, true, false) == 20.5f);
    assert(terrain_travel_take(origin, fresh, true, false) == 0.0f);
    assert(terrain_travel_take(origin, pending, false, true) == 0.0f);
    assert(origin[0] == pending[0] && origin[1] == pending[1]);
    assert(terrain_travel_take(origin, fresh, true, false) == 10.5f);
    assert(terrain_travel_take(origin, pending, true, true) == 0.0f);
    assert(origin[0] == pending[0] && origin[1] == pending[1]);
    assert(terrain_impact_pending(false, true, false));
    assert(terrain_impact_pending(true, false, false));
    assert(terrain_impact_pending(true, true, true));
    assert(!terrain_impact_pending(true, false, true));
    TerrainContext context;
    terrain_algo_init(&context);
    context.hump_count = 2u;
    context.max_hump_code = 20u;
    context.last_hump_code = 18u;
    context.state = TERRAIN_STATE_ON_HUMP;
    context.rise_q = 150u;
    terrain_algo_invalidate_window(&context);
    assert(context.state == TERRAIN_STATE_FLAT && context.rise_q == 0u);
    assert(context.hump_count == 2u && context.max_hump_code == 20u);
    assert(context.last_hump_code == 18u && !context.height_valid);
}

int main(void)
{
    test_evidence_and_invalid_window();
    test_complete_hump();
    test_real_crossing();
    test_a_dip_does_not_split_a_hump();
    test_what_is_not_a_hump();
    test_baseline();
    test_count();
    test_impact_and_restart();
    test_word_boundaries();
    puts("terrain: PASS");
    return 0;
}
