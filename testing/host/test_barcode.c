/* Host test for the barcode frame assembler and decoder (issue #35): both
   printed samples (A and Z) in both directions and scales, rejection paths,
   one event per print, chatter, steering, overflow of element width, reversed travel, and the three mapping
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
    barcode_frame_reset(&frame);
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
    /* P and C can complete a character early, with an unknown orientation. */
    assert(s == want && (want == 'P' || want == 'C' ||
        o == (reverse ? BARCODE_REVERSE : BARCODE_FORWARD)));
}

static void test_samples(void)
{
    /* 0xFFF0/0xFFE0 wrap inside the quiet zone; 0xFF00/0xFE80 wrap mid-frame. */
    const uint32_t bases[] = {0u, 0xFFF0u, 0xFFE0u, 0xFF00u, 0xFE80u};
    for (unsigned b = 0; b < sizeof bases / sizeof bases[0]; ++b) {
        for (int reverse = 0; reverse < 2; ++reverse) {
            sample("wnnnnwnnw", 'A', reverse, 30u, 90u, bases[b]);	/* small scale */
            sample("nwwnwnnnn", 'Z', reverse, 30u, 90u, bases[b]);
            sample("wnnnnwnnw", 'A', reverse, 80u, 240u, bases[b]);	/* about twice the scale */
            sample("nwwnwnnnn", 'Z', reverse, 80u, 240u, bases[b]);
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
            sample(pattern, (char)('A' + letter), reverse != 0u, 30u, 90u, 0xFFF0u);
            sample(pattern, (char)('A' + letter), reverse != 0u, 80u, 240u, 0xFF00u);
        }
    }
}

static void test_noisy_approach(void)
{
    uint32_t widths[29];
    char symbol = 0;
    barcode_orientation_t orientation = BARCODE_FORWARD;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false,
        30u, 90u, widths);
    start(0u);
    assert(step(100u, &symbol, &orientation) == BARCODE_NO_RESULT);
    assert(step(5u, &symbol, &orientation) == BARCODE_NO_RESULT);
    /* Reject the false frame, retaining this edge as the real frame start. */
    assert(step(BARCODE_MAX_ELEMENT_COUNTS + 1u, &symbol, &orientation) == BARCODE_BAD_WIDTH);
    barcode_result_t result = BARCODE_NO_RESULT;
    for (unsigned i = 0u; i < 29u; ++i)
    {
        result = step(widths[i], &symbol, &orientation);
        if (i + 1u < 29u)
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
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 30u, 90u, w);

    start(0u);						/* no quiet zone needed */
    assert(scan(w, 20u, &s, &o) == BARCODE_DECODED);

    uint32_t bad[29]; memcpy(bad, w, sizeof bad);
    bad[23] = 90u;					/* a wide where a narrow belongs */
    start(0u);
    barcode_result_t r = scan(bad, 100u, &s, &o);
    assert(r != BARCODE_DECODED);

    memcpy(bad, w, sizeof bad);
    bad[25] = 63u;					/* wide not clearly wider than narrow */
    start(0u);
    assert(scan(bad, 100u, &s, &o) != BARCODE_DECODED);

    memcpy(bad, w, sizeof bad);
    bad[12] = BARCODE_MAX_ELEMENT_COUNTS + 1u;		/* element too long */
    start(0u);
    assert(scan(bad, 100u, &s, &o) == BARCODE_BAD_WIDTH);

    /* Unknown symbol: valid frame shape whose pattern is not A-Z. */
    elements("nwnnwnwnn", "nnnnnnnnn", "nwnnwnwnn", false, 30u, 90u, bad);
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

/* One event per print: the window restarts after a decode. */
static void test_one_event_per_print(void)
{
    uint32_t w[29]; char s; barcode_orientation_t o;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 30u, 90u, w);
    start(0xFF00u);
    assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
    assert(frame.count == 0u);
    assert(step(30u, &s, &o) == BARCODE_NO_RESULT);
    assert(frame.count == 1u && frame.last_width == 30u);
    start(0xFF00u);
    assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
    assert(scan(w, 20u, &s, &o) == BARCODE_DECODED && s == 'A');
}

/* INT-137: sensor chatter inside a bar must not split the element. */
static void test_chatter(void)
{
    uint32_t w[29]; char s = 0; barcode_orientation_t o = BARCODE_FORWARD;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 60u, 180u, w);
    start(0u);
    assert(step(100u, &s, &o) == BARCODE_NO_RESULT);
    barcode_result_t r = BARCODE_NO_RESULT;
    for (unsigned i = 0u; i < 29u; ++i)
    {
        if (i == 4u || i == 13u)
        {
            assert(step(w[i] / 2u, &s, &o) == BARCODE_NO_RESULT);
            assert(step(8u, &s, &o) == BARCODE_NO_RESULT);	/* the chatter */
            r = step(w[i] - w[i] / 2u - 8u, &s, &o);
        }
        else
        {
            r = step(w[i], &s, &o);
        }
    }
    assert(r == BARCODE_DECODED && s == 'A');
}

/* INT-137: steering changes the mean of the wheels, not the sensor's travel. */
static void test_steering(void)
{
    uint32_t w[29]; char s = 0; barcode_orientation_t o = BARCODE_FORWARD;
    elements("nwnnwnwnn", "nwwnwnnnn", "nwnnwnwnn", false, 60u, 180u, w);
    start(0u);
    assert(step(100u, &s, &o) == BARCODE_NO_RESULT);
    barcode_result_t r = BARCODE_NO_RESULT;
    for (unsigned i = 0u; i < 29u; ++i)
    {
        /* The right wheel runs three times as far for every third element. */
        left = enc_advance(left, (int)w[i]);
        right = enc_advance(right, (int)(i % 3u == 0u ? w[i] * 3u : w[i]));
        black = !black;
        r = barcode_frame_edge(&frame, black, left, right, &s, &o);
    }
    assert(r == BARCODE_DECODED && s == 'Z');
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
            reverse != 0u, 30u, 90u, w);
        start(0xFFF0u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(s == 'Z');
        assert(o == (reverse != 0u ? BARCODE_REVERSE : BARCODE_FORWARD));
        assert(barcode_action_for(mapping, s) == BARCODE_U_TURN);

        /* Modest independent width jitter must preserve the same letter. */
        for (unsigned i = 0u; i < 29u; ++i)
        {
            w[i] = i % 2u == 0u ? w[i] - 1u : w[i] + 1u;
        }
        start(0xFF00u);
        assert(scan(w, 100u, &s, &o) == BARCODE_DECODED);
        assert(s == 'Z');
    }
}

/* INT-155 confirmed black high on real passes; inverted runs must not read. */
static void test_inverted_polarity(void)
{
    uint32_t w[29]; char s = 0; barcode_orientation_t o = BARCODE_FORWARD;
    elements("nwnnwnwnn", "wnnnnwnnw", "nwnnwnwnn", false, 60u, 180u, w);
    start(0u);
    black = true; /* the reference level is inverted */
    barcode_frame_reset(&frame);
    assert(barcode_frame_edge(&frame, black, left, right, &s, &o) == BARCODE_NO_RESULT);
    assert(scan(w, 100u, &s, &o) != BARCODE_DECODED);
}

/* S3-U37: real A widths, reads missed by the old three-widest rule: a narrow
   space (157) came within 1.5 times of the wide bars. */
static void test_real_pass(void)
{
    static const uint16_t widths[BARCODE_ELEMENTS] = {
        239, 91, 29, 259, 23, 115,
        186, 106, 32, 265, 32, 111, 33, 106, 197, 102,
        42, 157, 181, 81, 186, 106, 31, 259, 30};
    char s = 0;
    barcode_orientation_t o = BARCODE_FORWARD;
    assert(barcode_decode(widths, &s, &o) == BARCODE_DECODED);
    assert(s == 'A' && o == BARCODE_REVERSE);
}

/* INT-155: real passes just after a hump. A narrow space in the first guard read
   as black, so that guard arrived 7 elements long and never matched. */
static void test_hump_before_print(void)
{
    static const uint16_t passes[2][26] = {
        {130, 191, 103, 32, 276, 28, 106, 198, 88, 35, 259, 33, 106, 38, 103, 200,
            102, 38, 100, 197, 101, 199, 145, 36, 232, 27},
        {112, 219, 103, 29, 271, 30, 111, 182, 93, 35, 259, 32, 113, 38, 100, 199,
            104, 38, 102, 198, 102, 242, 102, 30, 233, 29}};
    for (unsigned p = 0u; p < 2u; ++p)
    {
        char s = 0;
        barcode_orientation_t o = BARCODE_FORWARD;
        barcode_result_t r = BARCODE_NO_RESULT;
        start(0u);
        assert(step(100u, &s, &o) == BARCODE_NO_RESULT);
        (void)step(p == 0u ? 435u : 354u, &s, &o); /* the merged bar */
        for (unsigned i = 0u; i < 26u; ++i)
        {
            r = step(passes[p][i], &s, &o);
            assert(i == 25u || r != BARCODE_DECODED);
        }
        assert(r == BARCODE_DECODED && s == 'A' && o == BARCODE_REVERSE);
    }
}

/* Dropping the first guard must not let any window inside a good print read as
   something else: every letter, both directions, decodes once and correctly. A
   lost run (three runs merged into one) may lose the read, never change the letter. */
static void test_no_early_read(void)
{
    for (unsigned letter = 0u; letter < BARCODE_ALPHABET; ++letter)
    {
        for (unsigned reverse = 0u; reverse < 2u; ++reverse)
        {
            static const uint16_t codes[BARCODE_ALPHABET] = {
                0x109u, 0x049u, 0x148u, 0x019u, 0x118u, 0x058u, 0x00du,
                0x10cu, 0x04cu, 0x01cu, 0x103u, 0x043u, 0x142u, 0x013u,
                0x112u, 0x052u, 0x007u, 0x106u, 0x046u, 0x016u, 0x181u,
                0x0c1u, 0x1c0u, 0x091u, 0x190u, 0x0d0u};
            char g2[10];
            for (unsigned i = 0u; i < 9u; ++i)
            {
                g2[i] = (codes[letter] >> (8u - i) & 1u) ? 'w' : 'n';
            }
            g2[9] = 0;
            uint32_t w[29];
            elements("nwnnwnwnn", g2, "nwnnwnwnn", reverse != 0u, 30u, 90u, w);
            char s = 0;
            barcode_orientation_t o = BARCODE_FORWARD;
            start(0u);
            assert(step(BARCODE_MAX_ELEMENT_COUNTS + 1u, &s, &o) == BARCODE_BAD_WIDTH);
            unsigned reads = 0u;
            for (unsigned i = 0u; i < 29u; ++i)
            {
                barcode_result_t r = step(w[i], &s, &o);
                reads += r == BARCODE_DECODED ? 1u : 0u;
            }
            assert(reads == 1u && s == (char)('A' + letter));
            for (unsigned m = 1u; m < 28u; ++m)
            {
                s = 0;
                start(0u);
                assert(step(BARCODE_MAX_ELEMENT_COUNTS + 1u, &s, &o) == BARCODE_BAD_WIDTH);
                for (unsigned i = 0u; i < 29u; ++i)
                {
                    if (i == m - 1u)
                    {
                        (void)step(w[i] + w[i + 1u] + w[i + 2u], &s, &o);
                        i += 2u;
                    }
                    else
                    {
                        (void)step(w[i], &s, &o);
                    }
                }
                assert(s == 0 || s == (char)('A' + letter));
            }
        }
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

static void test_chatter_filter(void)
{
    const barcode_count_t top = (barcode_count_t)ENC_COUNT_MASK;
    assert(barcode_run_is_chatter(100u, 100u, 100u, 100u)); /* wheels still */
    assert(barcode_run_is_chatter(100u, 100u, 100u + BARCODE_MIN_ELEMENT_COUNTS - 1u,
        100u + BARCODE_MIN_ELEMENT_COUNTS - 1u));
    assert(!barcode_run_is_chatter(100u, 100u, 100u + BARCODE_MIN_ELEMENT_COUNTS,
        100u + BARCODE_MIN_ELEMENT_COUNTS));
    /* The counters wrap, and a pivot runs the wheels opposite ways. */
    assert(barcode_run_is_chatter(top, top, 2u, 2u));
    assert(!barcode_run_is_chatter(top, top, 100u, 100u));
    assert(barcode_run_is_chatter(100u, 100u, 98u, 102u));
    assert(!barcode_run_is_chatter(1000u, 1000u, 900u, 1100u));
    puts("barcode chatter filter PASS");
}

/* Packed 9-bit widths: the window keeps the newest 25 exactly, including
   the minimum and maximum stored widths, as it shifts. */
static void test_packed_widths(void)
{
    uint32_t fed[40];
    char s; barcode_orientation_t o;
    start(0u);
    for (unsigned i = 0u; i < 40u; ++i)
    {
        fed[i] = i == 7u ? BARCODE_MIN_ELEMENT_COUNTS :
            (i % 5u == 0u ? BARCODE_MAX_ELEMENT_COUNTS :
            BARCODE_MIN_ELEMENT_COUNTS + (i * 97u) % 380u);
        (void)step(fed[i], &s, &o);
        unsigned stored = i + 1u < BARCODE_ELEMENTS ? i + 1u : BARCODE_ELEMENTS;
        assert(frame.count == stored);
        for (unsigned k = 0u; k < stored; ++k)
        {
            assert(barcode_frame_width(&frame, k) == fed[i + 1u - stored + k]);
        }
    }
}

int main(void)
{
    test_packed_widths();
    test_chatter_filter();
    test_samples();
    test_alphabet_vectors();
    test_noisy_approach();
    test_rejections();
    test_one_event_per_print();
    test_chatter();
    test_inverted_polarity();
    test_real_pass();
    test_hump_before_print();
    test_no_early_read();
    test_steering();
    test_flipped_capture();
    test_mapping();
    test_event_packing();
    puts("barcode: PASS");
    return 0;
}
