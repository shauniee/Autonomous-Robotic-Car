#ifndef BARCODE_TYPES_H
#define BARCODE_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

#define BARCODE_ELEMENTS 29u
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
    BARCODE_DISABLED,
    BARCODE_WAIT_QUIET,
    BARCODE_CAPTURE,
    BARCODE_WAIT_REARM,
    BARCODE_FAULT
} barcode_state_t;

typedef enum
{
    BARCODE_NO_RESULT,
    BARCODE_DECODED,
    BARCODE_BAD_CONFIG,
    BARCODE_BAD_ENCODER,
    BARCODE_BAD_EDGE,
    BARCODE_BAD_WIDTH,
    BARCODE_BAD_FRAME,
    BARCODE_BAD_SYMBOL,
    BARCODE_SHORT_QUIET,
    BARCODE_REVERSED_TRAVEL,
    BARCODE_TIMEOUT,
    BARCODE_CANCELLED,
    BARCODE_MAPPING_CHANGED,
    BARCODE_CAPTURE_OVERFLOW,
    BARCODE_EVENT_OVERFLOW
} barcode_result_t;

typedef enum
{
    BARCODE_FORWARD,
    BARCODE_REVERSE
} barcode_orientation_t;

/* Counts wrap modulo 2^32. Epoch changes on encoder reset/recalibration.
   sampled_us uses the shared hardware TIMER, NOT tk_get_otm milliseconds.
   valid means coherent capture, no missed encoder edges, usable calibration.
   Same timestamp is permitted when no movement occurred. */
typedef struct
{
    uint32_t left_count;
    uint32_t right_count;
    uint32_t sampled_us;
    uint32_t epoch;
    bool valid;
} barcode_encoder_t;

/* No measured defaults: every bound must be supplied before enabling.
   Micrometres/count includes quadrature decoding and gearing. Permille
   values express ratios/tolerances (3000 = 3:1, 100 = 10 percent). */
typedef struct
{
    uint32_t left_um_per_count;
    uint32_t right_um_per_count;
    uint32_t narrow_min_um;
    uint32_t narrow_max_um;
    uint32_t wide_ratio_min_milli;
    uint32_t wide_ratio_max_milli;
    uint32_t width_tolerance_milli;
    uint32_t quiet_zone_um;
    uint32_t rearm_um;
    uint32_t max_step_um;
    uint32_t max_wheel_difference_um;
    uint32_t max_frame_travel_um;
    uint32_t max_encoder_age_us;
    uint32_t max_edge_gap_us;
    uint32_t max_frame_us;
    bool black_high;
} barcode_config_t;

typedef struct
{
    uint32_t encounter_id;
    uint32_t sampled_us; /* Last black-to-white edge, not publication time. */
    uint32_t mapping_revision;
    uint32_t scanner_epoch;
    char symbol;
    barcode_action_t action;
    barcode_orientation_t orientation;
} barcode_event_t;

typedef struct
{
    barcode_state_t state;
    barcode_result_t result;
    barcode_event_t latest;
    uint32_t rejected_reads; /* Includes invalid idle observations. */
    uint32_t mapping_revision;
    uint32_t scanner_epoch;
    uint32_t fault_bits;
    uint8_t queued_events;
    bool have_decode;
} barcode_status_t;

#define BARCODE_FAULT_CAPTURE 1u
#define BARCODE_FAULT_QUEUE 2u

#endif
