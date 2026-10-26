#ifndef BARCODE_TYPES_H
#define BARCODE_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define BARCODE_ELEMENTS 29u	/* *X*: three 9-element characters and two gaps */
#define BARCODE_ALPHABET 26u

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

/* A decoded barcode with the encoder position where it ended. */
typedef struct
{
    uint32_t left_count, right_count;
    char symbol;
    barcode_action_t action;
    barcode_orientation_t orientation;
} barcode_event_t;

#endif
