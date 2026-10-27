/* Host test for app_program/subsystem3/ir_tracking.[ch] (spec #100, ticket
   #105): replays raw ADC pairs through the public calls (capture, sample,
   output, snapshot build) and checks the scaled-integer path against the
   float reference it replaced: identical line state, fractions, error and
   steering within one 1/127 step. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/subsystem3/ir_tracking.h"

/* ---- Float reference: the pre-#105 formulas ----------------------------- */

static float ref_clamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static float ref_fraction(uint16_t raw, uint16_t black, uint16_t white)
{
    return ref_clamp(((float)raw - (float)white) / ((float)black - (float)white), 0.0f, 1.0f);
}

static IrLine ref_classify(float l, float r, IrLine previous)
{
    if (l > IR_BROAD_BLACK && r > IR_BROAD_BLACK) return IR_LINE_BROAD_BLACK;
    if ((previous == IR_LINE_LOST && l < IR_WHITE_EXIT && r < IR_WHITE_EXIT) ||
        (l < IR_WHITE_ENTER && r < IR_WHITE_ENTER)) return IR_LINE_LOST;
    return IR_LINE_VISIBLE;
}

static float ref_steering(float error)
{
    return ref_clamp(IR_KP * error, -IR_MAX_CORRECTION, IR_MAX_CORRECTION) / IR_MAX_CORRECTION;
}

/* ---- Replay -------------------------------------------------------------- */

static uint32_t now;

static void feed(IrTracker *t, IrRaw raw)
{
    now += IR_SAMPLE_MS;
    ir_tracking_sample(t, raw, true, now);
}

/* Calibrate through the public capture path: black, then white. */
static void calibrate(IrTracker *t, IrRaw black, IrRaw white)
{
    ir_tracking_init(t);
    assert(ir_tracking_capture(t, IR_CAPTURE_BLACK, now));
    for (unsigned i = 0; i <= IR_CAPTURE_MS / IR_SAMPLE_MS; ++i) feed(t, black);
    assert(t->capture_result == IR_CAPTURE_BLACK_DONE);
    assert(ir_tracking_capture(t, IR_CAPTURE_WHITE, now));
    for (unsigned i = 0; i <= IR_CAPTURE_MS / IR_SAMPLE_MS; ++i) feed(t, white);
    assert(t->capture_result == IR_CAPTURE_WHITE_DONE);
}

static int near(float scaled, float reference)
{
    return fabsf(scaled - reference * IR_SCALE_ONE) <= 1.0f;
}

static unsigned replay(IrRaw black, IrRaw white)
{
    IrTracker t;
    calibrate(&t, black, white);
    IrLine ref_line = IR_LINE_UNKNOWN;
    unsigned steps = 0u, seq = 0u;
    /* Sweep both sensors across and beyond the calibrated span, in an order
       that crosses every threshold in both directions (hysteresis). */
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i <= 4095; i += 7) {
            int j = pass ? 4095 - i : (i * 3) % 4096;
            IrRaw raw = {(uint16_t)i, (uint16_t)j};
            feed(&t, raw);
            float lf = ref_fraction(raw.left, black.left, white.left);
            float rf = ref_fraction(raw.right, black.right, white.right);
            ref_line = ref_classify(lf, rf, ref_line);
            float error = rf - lf;

            IrTrackingOutput out = ir_tracking_output(&t, now);
            assert(out.line == ref_line);
            assert(out.valid == (ref_line == IR_LINE_VISIBLE));
            if (out.valid) {
                assert(near(out.error, error));
                assert(near(out.steering, ref_steering(error)));
            } else {
                assert(out.error == 0 && out.steering == 0);
            }

            S3IrSnapshot s = ir_snapshot_build(&t, ++seq, now);
            assert(s.normalized_valid && s.line == ref_line && s.guidance_valid == out.valid);
            assert(fabsf(s.left_fraction - lf * IR_SCALE_ONE) <= 0.5f);	/* nearest step */
            assert(fabsf(s.right_fraction - rf * IR_SCALE_ONE) <= 0.5f);
            assert(near(s.error, error) && s.steering == out.steering);
            assert(s.raw.left == raw.left && s.raw.right == raw.right);
            ++steps;
        }
    }
    return steps;
}

static void test_bounds_and_staleness(void)
{
    IrTracker t;
    calibrate(&t, (IrRaw){3500u, 3400u}, (IrRaw){400u, 500u});
    feed(&t, (IrRaw){4095u, 0u});		/* full black left, full white right */
    S3IrSnapshot s = ir_snapshot_build(&t, 1u, now);
    assert(s.left_fraction == IR_SCALE_ONE && s.right_fraction == 0u && s.error == -IR_SCALE_ONE);
    feed(&t, (IrRaw){0u, 4095u});
    s = ir_snapshot_build(&t, 2u, now);
    assert(s.error == IR_SCALE_ONE);
    if (s.guidance_valid) assert(s.steering > 0);	/* positive means right */
    /* A stale sample clears the scaled fields. */
    ir_snapshot_refresh(&s, now + IR_MAX_GAP_MS + 1u);
    assert(!s.normalized_valid && s.left_fraction == 0u && s.error == 0 && s.steering == 0);
}

int main(void)
{
    unsigned steps = 0u;
    steps += replay((IrRaw){3500u, 3400u}, (IrRaw){400u, 500u});	/* high on black */
    steps += replay((IrRaw){300u, 450u}, (IrRaw){3900u, 3700u});	/* inverted polarity */
    steps += replay((IrRaw){2100u, 2300u}, (IrRaw){1700u, 1900u});	/* low contrast */
    test_bounds_and_staleness();
    printf("ir_tracking: PASS (%u replayed samples)\n", steps);
    return 0;
}
