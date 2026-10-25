/* Host test for the barcode frame assembler and decoder (issue #35): both
   printed samples (A and Z) in both directions and scales, rejection paths,
   rearm, overflow of element width, reversed travel, and the three mapping
   examples from the specs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/subsystem3/barcode_frame.h"
#include "../../app_program/subsystem2/encoder_math.h"
#include "../../app_program/common/robot_data.h"

static barcode_frame_t frame;
static uint32_t left, right;
static bool black;

static void start(uint32_t base)
{
    const barcode_config_t cfg = {BARCODE_NARROW_MIN, BARCODE_NARROW_MAX, BARCODE_WIDE_RATIO_MIN_MILLI,
                                  BARCODE_WIDE_RATIO_MAX_MILLI, BARCODE_TOLERANCE_MILLI};
    barcode_frame_init(&frame, &cfg);
    left = right = base & ENC_COUNT_MASK;
    black = false;
    char s; barcode_orientation_t o;
    assert(barcode_frame_edge(&frame, black, left, right, &s, &o) == BARCODE_NO_RESULT);	/* reference edge */
}

static barcode_result_t step(uint32_t width, char *s, barcode_orientation_t *o)
{
    left = enc_advance(left, (int)width);
    right = enc_advance(right, (int)width);
    black = !black;
    return barcode_frame_edge(&frame, black, left, right, s, o);
}

/* Elements of *X*: groups of 9 with narrow inter-character gaps. */
static void elements(const char *g1, const char *g2, const char *g3, bool reverse, unsigned n, unsigned w, uint32_t out[29])
{
    char all[30];
    snprintf(all, sizeof all, "%sn%sn%s", g1, g2, g3);
    assert(strlen(all) == 29u);
    for (unsigned i = 0; i < 29u; ++i) {
        char c = reverse ? all[28u - i] : all[i];
        out[i] = c == 'w' ? w : n;
    }
}

static barcode_result_t scan(const uint32_t widths[29], uint32_t quiet, char *s, barcode_orientation_t *o)
{
    barcode_result_t r = step(quiet, s, o);			/* white run -> black */
    assert(r == BARCODE_NO_RESULT);
    for (unsigned i = 0; i < 29u; ++i) {
        r = step(widths[i], s, o);
        if (i < 28u && r != BARCODE_NO_RESULT) return r;
    }
    return r;
}

static void sample(const char *g2, char want, bool reverse, unsigned n, unsigned w, uint32_t base)
{
    uint32_t widths[29]; char s = 0; barcode_orientation_t o = BARCODE_FORWARD;
    elements("nwnnwnwnn", g2, "nwnnwnwnn", reverse, n, w, widths);
    start(base);
    assert(scan(widths, 100u, &s, &o) == BARCODE_DECODED);
    assert(s == want && o == (reverse ? BARCODE_REVERSE : BARCODE_FORWARD));
}

static void test_samples(void)
{
    const uint32_t bases[] = {0u, 0xFFF0u, 0xFFE0u};	/* includes the count wrap */
    for (unsigned b = 0; b < 3u; ++b) {
        for (int reverse = 0; reverse < 2; ++reverse) {
            sample("wnnnnwnnw", 'A', reverse, 10u, 30u, bases[b]);	/* small scale */
            sample("nwwnwnnnn", 'Z', reverse, 10u, 30u, bases[b]);
            sample("wnnnnwnnw", 'A', reverse, 20u, 60u, bases[b]);	/* about twice the scale */
            sample("nwwnwnnnn", 'Z', reverse, 20u, 60u, bases[b]);
        }
    }
}

static void test_rejections(void)
{
    uint32_t w[29]; char s; barcode_orientation_t o;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 10u, 30u, w);

    start(0u);						/* short quiet zone: never starts a frame */
    assert(scan(w, 60u, &s, &o) == BARCODE_NO_RESULT);

    uint32_t bad[29]; memcpy(bad, w, sizeof bad);
    bad[3] = 30u;					/* a wide where a narrow belongs */
    start(0u);
    barcode_result_t r = scan(bad, 100u, &s, &o);
    assert(r == BARCODE_BAD_FRAME || r == BARCODE_BAD_WIDTH);

    memcpy(bad, w, sizeof bad);
    bad[5] = 19u;					/* neither narrow nor wide at tolerance */
    start(0u);
    assert(scan(bad, 100u, &s, &o) == BARCODE_BAD_WIDTH);

    memcpy(bad, w, sizeof bad);
    bad[12] = BARCODE_MAX_ELEMENT_COUNTS + 1u;		/* element too long */
    start(0u);
    assert(scan(bad, 100u, &s, &o) == BARCODE_BAD_WIDTH);

    /* Unknown symbol: valid frame shape whose pattern is not A-Z. */
    elements("nwnnwnwnn", "nnnnnnnnn", "nwnnwnwnn", false, 10u, 30u, bad);
    start(0u);
    assert(scan(bad, 100u, &s, &o) != BARCODE_DECODED);

    /* Reversed encoder travel during a frame. */
    start(0u);
    assert(step(100u, &s, &o) == BARCODE_NO_RESULT);
    left = enc_advance(left, -5); right = enc_advance(right, -5);
    black = !black;
    assert(barcode_frame_edge(&frame, black, left, right, &s, &o) == BARCODE_REVERSED_TRAVEL);

    /* Missed edge (same polarity twice) resynchronises. */
    start(0u);
    assert(barcode_frame_edge(&frame, false, left + 10u, right + 10u, &s, &o) == BARCODE_BAD_FRAME);
}

static void test_rearm(void)
{
    uint32_t w[29]; char s; barcode_orientation_t o;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 10u, 30u, w);
    start(0u);
    assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
    /* A second frame after too small a white gap is not accepted (duplicate rearm). */
    assert(scan(w, BARCODE_REARM_COUNTS - 20u, &s, &o) != BARCODE_DECODED);
    /* After a full gap the next barcode decodes again. Resynchronise first. */
    start(0u);
    assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
    assert(scan(w, BARCODE_REARM_COUNTS + 20u, &s, &o) == BARCODE_DECODED);
}

static void test_mapping(void)
{
    struct { uint8_t letters[4]; char probe[4]; barcode_action_t want[4]; } cases[] = {
        {{'A'-'A', 'B'-'A', 'C'-'A', 'D'-'A'}, {'A','B','C','D'}, {BARCODE_LEFT, BARCODE_RIGHT, BARCODE_STRAIGHT, BARCODE_U_TURN}},
        {{'B'-'A', 'C'-'A', 'A'-'A', 'D'-'A'}, {'A','B','C','D'}, {BARCODE_STRAIGHT, BARCODE_LEFT, BARCODE_RIGHT, BARCODE_U_TURN}},
        {{'W'-'A', 'F'-'A', 'A'-'A', 'Z'-'A'}, {'A','Z','W','F'}, {BARCODE_STRAIGHT, BARCODE_U_TURN, BARCODE_LEFT, BARCODE_RIGHT}},
    };
    for (unsigned c = 0; c < 3u; ++c) {
        uint32_t word;
        assert(rd_mapping_pack(cases[c].letters, &word));
        for (unsigned i = 0; i < 4u; ++i) {
            assert(barcode_action_for(word, cases[c].probe[i]) == cases[c].want[i]);
        }
        assert(barcode_action_for(word, 'Q') == BARCODE_UNMAPPED);	/* letter not mapped */
        assert(barcode_action_for(word, '1') == BARCODE_UNMAPPED);	/* not A-Z */
    }
    assert(barcode_action_for(0u, 'A') == BARCODE_UNMAPPED);		/* boot state */
}

int main(void)
{
    test_samples();
    test_rejections();
    test_rearm();
    test_mapping();
    puts("barcode: PASS");
    return 0;
}
