/* Host test for the barcode frame assembler and decoder (issue #35): both
   printed samples (A and Z) in both directions and scales, rejection paths,
   rearm, overflow of element width, reversed travel, and the three mapping
   examples from the specs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/line_barcode/line_barcode.h"
#include "../../app_program/motion/motion.h"
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
    /* 0xFFF0/0xFFE0 wrap inside the quiet zone; 0xFF00/0xFE80 wrap mid-frame. */
    const uint32_t bases[] = {0u, 0xFFF0u, 0xFFE0u, 0xFF00u, 0xFE80u};
    for (unsigned b = 0; b < sizeof bases / sizeof bases[0]; ++b) {
        for (int reverse = 0; reverse < 2; ++reverse) {
            sample("wnnnnwnnw", 'A', reverse, 10u, 30u, bases[b]);	/* small scale */
            sample("nwwnwnnnn", 'Z', reverse, 10u, 30u, bases[b]);
            sample("wnnnnwnnw", 'A', reverse, 20u, 60u, bases[b]);	/* about twice the scale */
            sample("nwwnwnnnn", 'Z', reverse, 20u, 60u, bases[b]);
        }
    }
}

static void test_alphabet_vectors(void)
{
    /* Canonical Code 39 A-Z encodings, independently checked against ZXing's
       CHARACTER_ENCODINGS (not obtained by calling this decoder's table):
       https://github.com/zxing/zxing/blob/master/core/src/main/java/com/google/zxing/oned/Code39Reader.java */
    static const uint16_t reference[] = {
        0x109u, 0x049u, 0x148u, 0x019u, 0x118u, 0x058u, 0x00Du,
        0x10Cu, 0x04Cu, 0x01Cu, 0x103u, 0x043u, 0x142u, 0x013u,
        0x112u, 0x052u, 0x007u, 0x106u, 0x046u, 0x016u, 0x181u,
        0x0C1u, 0x1C0u, 0x091u, 0x190u, 0x0D0u};
    for (unsigned letter = 0u; letter < 26u; ++letter)
    {
        char pattern[10];
        for (unsigned element = 0u; element < 9u; ++element)
        {
            pattern[element] = (reference[letter] & (1u << (8u - element))) != 0u ? 'w' : 'n';
        }
        pattern[9] = '\0';
        for (unsigned reverse = 0u; reverse < 2u; ++reverse)
        {
            sample(pattern, (char)('A' + letter), reverse != 0u, 10u, 30u, 0xFFF0u);
            sample(pattern, (char)('A' + letter), reverse != 0u, 20u, 60u, 0xFF00u);
        }
    }
}

static void test_noisy_approach(void)
{
    uint32_t widths[29];
    char symbol = 0;
    barcode_orientation_t orientation = BARCODE_FORWARD;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false,
        10u, 30u, widths);
    start(0u);
    assert(step(100u, &symbol, &orientation) == BARCODE_NO_RESULT);
    assert(step(5u, &symbol, &orientation) == BARCODE_NO_RESULT);
    /* Reject the false frame, retaining this edge as the real frame start. */
    assert(step(200u, &symbol, &orientation) == BARCODE_BAD_WIDTH);
    barcode_result_t result = BARCODE_NO_RESULT;
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        result = step(widths[i], &symbol, &orientation);
        if (i + 1u < BARCODE_ELEMENTS)
        {
            assert(result == BARCODE_NO_RESULT);
        }
    }
    assert(result == BARCODE_DECODED);
    assert(symbol == 'A' && orientation == BARCODE_FORWARD);
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
    for (unsigned reverse = 0u; reverse < 2u; ++reverse)
    {
        elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn",
            reverse != 0u, 10u, 30u, w);
        start(0xFF00u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(scan(w, BARCODE_REARM_COUNTS - 20u, &s, &o) != BARCODE_DECODED);
        start(0xFF00u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(scan(w, BARCODE_REARM_COUNTS + 20u, &s, &o) == BARCODE_DECODED);
        assert(s == 'A');
        assert(o == (reverse != 0u ? BARCODE_REVERSE : BARCODE_FORWARD));
    }
}

static void test_flipped_capture(void)
{
    uint32_t w[29];
    char s = '?';
    barcode_orientation_t o = BARCODE_FORWARD;
    const uint8_t letters[4] = {0u, 1u, 2u, 25u};
    uint32_t mapping;
    assert(rd_mapping_pack(letters, &mapping));
    for (unsigned reverse = 0u; reverse < 2u; ++reverse)
    {
        elements("nwnnwnwnn", "nwwnwnnnn", "nwnnwnwnn",
            reverse != 0u, 10u, 30u, w);
        /* Without the enable-time reference, the first black edge loses the
           approach quiet zone and this first complete print is missed. */
        start(0xFFF0u);
        barcode_frame_reset(&frame);
        assert(scan(w, 100u, &s, &o) != BARCODE_DECODED);
        /* start() supplies the reference now queued by drv_barcode_enable. */
        start(0xFFF0u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(s == 'Z');
        assert(o == (reverse != 0u ? BARCODE_REVERSE : BARCODE_FORWARD));
        assert(barcode_action_for(mapping, s) == BARCODE_U_TURN);

        /* Modest independent width jitter must preserve the same letter. */
        for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
        {
            w[i] = i % 2u == 0u ? w[i] - 1u : w[i] + 1u;
        }
        start(0xFF00u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(s == 'Z');

        /* Opposite orientations on the two guards must never guess a letter. */
        elements("nwnnwnwnn", "nwwnwnnnn", "nnwnwnnwn",
            reverse != 0u, 10u, 30u, w);
        start(0u);
        s = '?';
        o = BARCODE_FORWARD;
        assert(scan(w, 100u, &s, &o) == BARCODE_BAD_FRAME);
        assert(s == '?' && o == BARCODE_FORWARD);
    }
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

static void test_event_packing(void)
{
    const barcode_orientation_t orients[] = {BARCODE_FORWARD, BARCODE_REVERSE};
    for (int c = 'A' - 1; c <= 'Z' + 1; ++c) {		/* '@' and '[' are "no letter" */
        for (int a = BARCODE_UNMAPPED; a <= BARCODE_U_TURN; ++a) {
            for (unsigned k = 0; k < 2u; ++k) {
                barcode_event_t e = {.left_count = 0xFFFFu, .right_count = 0x0001u};
                barcode_event_set(&e, (char)c, (barcode_action_t)a, orients[k]);
                uint8_t want = c >= 'A' && c <= 'Z' ? (uint8_t)(c - 'A') : BARCODE_NO_LETTER;
                assert(barcode_event_letter(&e) == want);
                assert(barcode_event_action(&e) == (barcode_action_t)a);
                assert(barcode_event_orientation(&e) == orients[k]);
                assert(e.left_count == 0xFFFFu && e.right_count == 0x0001u);	/* counts untouched */
            }
        }
    }
    barcode_event_t e;
    barcode_event_set(&e, '\0', (barcode_action_t)7, BARCODE_FORWARD);	/* out-of-range action */
    assert(barcode_event_letter(&e) == BARCODE_NO_LETTER && barcode_event_action(&e) == BARCODE_UNMAPPED);
    _Static_assert(sizeof(barcode_event_t) == 6u, "barcode event is two counts plus one packed word");
}

int main(void)
{
    test_samples();
    test_alphabet_vectors();
    test_noisy_approach();
    test_rejections();
    test_rearm();
    test_flipped_capture();
    test_mapping();
    test_event_packing();
    puts("barcode: PASS");
    return 0;
}
