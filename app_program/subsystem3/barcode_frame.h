/*
 *	barcode_frame.h
 *	Kernel-free frame assembler: turns edges (level plus the two wheel
 *	encoder counts at the edge) into element widths measured in encoder
 *	travel, waits for a quiet zone, and decodes 29 elements.
 */

#ifndef BARCODE_FRAME_H
#define BARCODE_FRAME_H

#include "barcode_decode.h"

typedef enum { BARCODE_FRAME_WAIT_QUIET, BARCODE_FRAME_CAPTURE } barcode_frame_state_t;

typedef struct
{
    barcode_config_t config;
    barcode_frame_state_t state;
    uint32_t widths[BARCODE_ELEMENTS];
    uint32_t count;
    uint32_t min_quiet;	/* quiet zone now required: quiet, or rearm after a decode */
    uint32_t prev_left, prev_right;
    bool have_edge;
    bool prev_black;
} barcode_frame_t;

void barcode_frame_init(barcode_frame_t *f, const barcode_config_t *config);
void barcode_frame_reset(barcode_frame_t *f);

/* Feed one edge. `black` is the level after the edge. Returns BARCODE_DECODED
   (symbol/orientation set) when a frame completes, BARCODE_NO_RESULT while
   accumulating or waiting, otherwise the rejection reason (frame dropped). */
barcode_result_t barcode_frame_edge(barcode_frame_t *f, bool black,
    uint32_t left_count, uint32_t right_count, char *symbol,
    barcode_orientation_t *orientation);

/* Action for a decoded letter under a mapping word (BARCODE_UNMAPPED when
   the word is unmapped or the letter is not one of the four). */
barcode_action_t barcode_action_for(uint32_t mapping_word, char symbol);

#endif
