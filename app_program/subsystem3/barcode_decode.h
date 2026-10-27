/*
 *	barcode_decode.h
 *	Kernel-free barcode types, Code 39 decoder and frame assembler: turns edges
 *	(level plus the two wheel encoder counts at the edge) into element widths
 *	measured in encoder travel, waits for a quiet zone, and decodes 29 elements.
 */

#ifndef BARCODE_DECODE_H
#define BARCODE_DECODE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define BARCODE_ELEMENTS 29u	/* *X*: three 9-element characters and two gaps */
#define BARCODE_ALPHABET 26u

/* Encoder counts as S3 stores them: the shared ENC_COUNT_BITS width, wrapped.
   Travel between two counts uses modular subtraction (encoder_math.h). */
typedef uint16_t barcode_count_t;
_Static_assert(ENC_COUNT_BITS <= 16u, "barcode_count_t narrower than ENC_COUNT_BITS");
/* The longest barcode (29 elements at the longest width) spans less than half
   the count range, so modular subtraction inside a frame cannot alias. A wider
   or narrower ENC_COUNT_BITS (D03) re-checks this bound. */
_Static_assert((uint32_t)BARCODE_ELEMENTS * BARCODE_MAX_ELEMENT_COUNTS < ENC_COUNT_MASK / 2u,
    "longest barcode must span less than half the encoder count range");

typedef enum
{
    BARCODE_UNMAPPED,
    BARCODE_LEFT,
    BARCODE_RIGHT,
    BARCODE_STRAIGHT,
    BARCODE_U_TURN
} barcode_action_t;

typedef enum
{
    BARCODE_NO_RESULT,
    BARCODE_DECODED,
    BARCODE_BAD_CONFIG,
    BARCODE_BAD_WIDTH,
    BARCODE_BAD_FRAME,
    BARCODE_BAD_SYMBOL,
    BARCODE_REVERSED_TRAVEL
} barcode_result_t;

typedef enum
{
    BARCODE_FORWARD,
    BARCODE_REVERSE
} barcode_orientation_t;

/* Widths and limits are in encoder counts (mean of both wheels). Permille
   values express ratios/tolerances (3000 = 3:1, 300 = 30 percent). */
typedef struct
{
    uint32_t narrow_min;
    uint32_t narrow_max;
    uint32_t wide_ratio_min_milli;
    uint32_t wide_ratio_max_milli;
    uint32_t width_tolerance_milli;
} barcode_config_t;

/* A decoded barcode with the encoder position where it ended. `packed` holds
   letter (bits 0-4: 0-25, BARCODE_NO_LETTER), action (bits 5-7) and
   orientation (bit 8); use the accessors below, never mask by hand. */
typedef struct
{
    barcode_count_t left_count, right_count;
    uint16_t packed;
} barcode_event_t;

#define BARCODE_NO_LETTER 31u
#define BARCODE_EVENT_ACTION_SHIFT 5u
#define BARCODE_EVENT_ORIENTATION_SHIFT 8u

/* Letter 'A'-'Z' maps to 0-25; anything else stores BARCODE_NO_LETTER. An
   action outside barcode_action_t stores BARCODE_UNMAPPED. */
static inline void barcode_event_set(barcode_event_t *e, char symbol,
    barcode_action_t action, barcode_orientation_t orientation)
{
    unsigned letter = symbol >= 'A' && symbol <= 'Z' ? (unsigned)(symbol - 'A') : BARCODE_NO_LETTER;
    unsigned a = (unsigned)action <= (unsigned)BARCODE_U_TURN ? (unsigned)action : (unsigned)BARCODE_UNMAPPED;
    e->packed = (uint16_t)(letter | a << BARCODE_EVENT_ACTION_SHIFT |
        (orientation == BARCODE_REVERSE ? 1u : 0u) << BARCODE_EVENT_ORIENTATION_SHIFT);
}

/* 0-25 for 'A'-'Z', BARCODE_NO_LETTER otherwise. */
static inline uint8_t barcode_event_letter(const barcode_event_t *e)
{
    return (uint8_t)(e->packed & 0x1Fu);
}

static inline barcode_action_t barcode_event_action(const barcode_event_t *e)
{
    return (barcode_action_t)(e->packed >> BARCODE_EVENT_ACTION_SHIFT & 0x7u);
}

static inline barcode_orientation_t barcode_event_orientation(const barcode_event_t *e)
{
    return (e->packed >> BARCODE_EVENT_ORIENTATION_SHIFT & 1u) ? BARCODE_REVERSE : BARCODE_FORWARD;
}

bool barcode_config_valid(const barcode_config_t *config);
/* Pure, bounded decoder. Failure leaves destinations unchanged. */
barcode_result_t barcode_decode(const uint32_t widths[BARCODE_ELEMENTS],
    const barcode_config_t *config, char *symbol,
    barcode_orientation_t *orientation);

typedef enum { BARCODE_FRAME_WAIT_QUIET, BARCODE_FRAME_CAPTURE } barcode_frame_state_t;

typedef struct
{
    barcode_config_t config;
    barcode_frame_state_t state;
    uint32_t widths[BARCODE_ELEMENTS];
    uint32_t count;
    uint32_t min_quiet;	/* quiet zone now required: quiet, or rearm after a decode */
    barcode_count_t prev_left, prev_right;
    bool have_edge;
    bool prev_black;
} barcode_frame_t;

void barcode_frame_init(barcode_frame_t *f, const barcode_config_t *config);
void barcode_frame_reset(barcode_frame_t *f);

/* Feed one edge. `black` is the level after the edge. Returns BARCODE_DECODED
   (symbol/orientation set) when a frame completes, BARCODE_NO_RESULT while
   accumulating or waiting, otherwise the rejection reason (frame dropped). */
barcode_result_t barcode_frame_edge(barcode_frame_t *f, bool black,
    barcode_count_t left_count, barcode_count_t right_count, char *symbol,
    barcode_orientation_t *orientation);

/* Action for a decoded letter under a mapping word (BARCODE_UNMAPPED when
   the word is unmapped or the letter is not one of the four). */
barcode_action_t barcode_action_for(uint32_t mapping_word, char symbol);

#endif
