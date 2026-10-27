#include "barcode_decode.h"
#include <stddef.h>
#include "../subsystem2/encoder_math.h"
#include "../common/robot_data.h"

/* Code 39 standard patterns: bit 8 is the first element, 1 means wide.
   Verified against the A/Z project patterns and ZXing Code39Reader's
   published encoding table. Only A-Z plus framing '*' are supported. */
static const uint16_t alphabet[BARCODE_ALPHABET] =
{
    0x109u, 0x049u, 0x148u, 0x019u, 0x118u, 0x058u, 0x00du,
    0x10cu, 0x04cu, 0x01cu, 0x103u, 0x043u, 0x142u, 0x013u,
    0x112u, 0x052u, 0x007u, 0x106u, 0x046u, 0x016u, 0x181u,
    0x0c1u, 0x1c0u, 0x091u, 0x190u, 0x0d0u
};

bool barcode_config_valid(const barcode_config_t *c)
{
    if ((c == NULL) || (c->narrow_min == 0u) ||
        (c->narrow_min > c->narrow_max) ||
        (c->wide_ratio_min_milli <= 1000u) ||
        (c->wide_ratio_min_milli > c->wide_ratio_max_milli) ||
        (c->wide_ratio_max_milli > 10000u) ||
        (c->width_tolerance_milli == 0u) ||
        (c->width_tolerance_milli >= 500u))
    {
        return false;
    }
    /* Classes must not overlap even at the configured tolerance edges. */
    return (uint64_t)c->wide_ratio_min_milli *
        (1000u - c->width_tolerance_milli) >
        (uint64_t)1000u * (1000u + c->width_tolerance_milli);
}

static uint16_t reverse_pattern(uint16_t pattern)
{
    uint16_t reversed = 0u;
    for (unsigned i = 0u; i < 9u; ++i)
    {
        reversed = (uint16_t)((reversed << 1) | (pattern & 1u));
        pattern >>= 1;
    }
    return reversed;
}

static bool fits(uint32_t width, uint32_t reference, uint32_t tolerance)
{
    uint64_t difference = width > reference ? width - reference :
        reference - width;
    return difference * 1000u <= (uint64_t)reference * tolerance;
}

barcode_result_t barcode_decode(const uint32_t widths[BARCODE_ELEMENTS],
    const barcode_config_t *config, char *symbol,
    barcode_orientation_t *orientation)
{
    if ((widths == NULL) || (symbol == NULL) || (orientation == NULL) ||
        !barcode_config_valid(config))
    {
        return BARCODE_BAD_CONFIG;
    }
    uint32_t sorted[BARCODE_ELEMENTS];
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        sorted[i] = widths[i];
        for (unsigned j = i; (j > 0u) && (sorted[j] < sorted[j - 1u]); --j)
        {
            uint32_t value = sorted[j];
            sorted[j] = sorted[j - 1u];
            sorted[j - 1u] = value;
        }
    }
    /* *X*: twenty narrow elements including gaps, nine wide elements. */
    uint32_t narrow = (uint32_t)(((uint64_t)sorted[9] + sorted[10]) / 2u);
    uint32_t wide = sorted[24];
    if ((narrow < config->narrow_min) ||
        (narrow > config->narrow_max) || (sorted[19] >= sorted[20]) ||
        ((uint64_t)wide * 1000u <
            (uint64_t)narrow * config->wide_ratio_min_milli) ||
        ((uint64_t)wide * 1000u >
            (uint64_t)narrow * config->wide_ratio_max_milli))
    {
        return BARCODE_BAD_WIDTH;
    }
    uint16_t patterns[3] = {0};
    unsigned wide_counts[3] = {0};
    for (unsigned i = 0u; i < BARCODE_ELEMENTS; ++i)
    {
        bool is_narrow = fits(widths[i], narrow,
            config->width_tolerance_milli);
        bool is_wide = fits(widths[i], wide, config->width_tolerance_milli);
        if (is_narrow == is_wide)
        {
            return BARCODE_BAD_WIDTH;
        }
        if ((i == 9u) || (i == 19u))
        {
            if (!is_narrow)
            {
                return BARCODE_BAD_FRAME;
            }
            continue;
        }
        unsigned group = i / 10u;
        patterns[group] = (uint16_t)((patterns[group] << 1) |
            (is_wide ? 1u : 0u));
        wide_counts[group] += is_wide ? 1u : 0u;
    }
    if ((wide_counts[0] != 3u) || (wide_counts[1] != 3u) ||
        (wide_counts[2] != 3u))
    {
        return BARCODE_BAD_FRAME;
    }
    const uint16_t star = 0x094u;
    barcode_orientation_t direction;
    uint16_t payload = patterns[1];
    if ((patterns[0] == star) && (patterns[2] == star))
    {
        direction = BARCODE_FORWARD;
    }
    else if ((patterns[0] == reverse_pattern(star)) &&
        (patterns[2] == reverse_pattern(star)))
    {
        direction = BARCODE_REVERSE;
        payload = reverse_pattern(payload);
    }
    else
    {
        return BARCODE_BAD_FRAME;
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        if (alphabet[i] == payload)
        {
            *symbol = (char)('A' + i);
            *orientation = direction;
            return BARCODE_DECODED;
        }
    }
    return BARCODE_BAD_SYMBOL;
}

barcode_action_t barcode_action_for(uint32_t word, char symbol)
{
    rd_map_action_t action;
    if (symbol < 'A' || symbol > 'Z' || !rd_mapping_lookup(word, (uint8_t)(symbol - 'A'), &action))
    {
        return BARCODE_UNMAPPED;
    }
    switch (action)
    {
    case RD_MAP_LEFT: return BARCODE_LEFT;
    case RD_MAP_RIGHT: return BARCODE_RIGHT;
    case RD_MAP_STRAIGHT: return BARCODE_STRAIGHT;
    default: return BARCODE_U_TURN;
    }
}

void barcode_frame_reset(barcode_frame_t *f)
{
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    f->min_quiet = BARCODE_QUIET_COUNTS;
    f->have_edge = false;
}

void barcode_frame_init(barcode_frame_t *f, const barcode_config_t *config)
{
    *f = (barcode_frame_t){.config = *config};
    barcode_frame_reset(f);
}

static void drop(barcode_frame_t *f)
{
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    f->min_quiet = BARCODE_QUIET_COUNTS;
}

barcode_result_t barcode_frame_edge(barcode_frame_t *f, bool black,
    barcode_count_t left_count, barcode_count_t right_count, char *symbol,
    barcode_orientation_t *orientation)
{
    if (!f->have_edge)
    {
        f->have_edge = true;
        f->prev_left = left_count;
        f->prev_right = right_count;
        f->prev_black = black;
        return BARCODE_NO_RESULT;
    }
    int32_t left = enc_travel_signed(left_count, f->prev_left);
    int32_t right = enc_travel_signed(right_count, f->prev_right);
    bool run_was_black = f->prev_black;
    f->prev_left = left_count;
    f->prev_right = right_count;
    f->prev_black = black;
    if (black == run_was_black)
    {
        /* Two edges of one polarity: an edge was missed. Resynchronise. */
        drop(f);
        return BARCODE_BAD_FRAME;
    }
    if (left < 0 || right < 0)
    {
        drop(f);
        return BARCODE_REVERSED_TRAVEL;
    }
    uint32_t width = ((uint32_t)left + (uint32_t)right) / 2u;

    if (f->state == BARCODE_FRAME_WAIT_QUIET)
    {
        /* A white run that is long enough, ended by a black edge, starts a frame. */
        if (!run_was_black && black && width >= f->min_quiet)
        {
            f->state = BARCODE_FRAME_CAPTURE;
            f->count = 0u;
        }
        return BARCODE_NO_RESULT;
    }

    if (width > BARCODE_MAX_ELEMENT_COUNTS)
    {
        drop(f);
        return BARCODE_BAD_WIDTH;
    }
    f->widths[f->count++] = width;
    if (f->count < BARCODE_ELEMENTS)
    {
        return BARCODE_NO_RESULT;
    }
    barcode_result_t result = barcode_decode(f->widths, &f->config, symbol, orientation);
    f->state = BARCODE_FRAME_WAIT_QUIET;
    f->count = 0u;
    /* After a decode demand the longer rearm gap before the next frame. */
    f->min_quiet = result == BARCODE_DECODED ? BARCODE_REARM_COUNTS : BARCODE_QUIET_COUNTS;
    return result;
}
