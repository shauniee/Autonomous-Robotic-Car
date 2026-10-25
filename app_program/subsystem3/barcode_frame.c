#include "barcode_frame.h"
#include "../subsystem2/encoder_math.h"
#include "../common/robot_data.h"

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
    uint32_t left_count, uint32_t right_count, char *symbol,
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
