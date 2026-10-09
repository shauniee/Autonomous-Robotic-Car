#ifndef S3_BARCODE_H
#define S3_BARCODE_H
#include <stdbool.h>
#include <stdint.h>

#define S3_BARCODE_EDGE_CAPACITY 256u
#define S3_NAV_CAPACITY 4u
#define S3_BARCODE_WIDTHS 29u

typedef enum { S3_NAV_NONE, S3_NAV_LEFT, S3_NAV_RIGHT,
    S3_NAV_STRAIGHT, S3_NAV_UTURN } S3NavAction;
typedef enum { S3_BC_DISABLED, S3_BC_QUIET, S3_BC_CAPTURE,
    S3_BC_REARM, S3_BC_FAULT } S3BarcodeState;
typedef enum { S3_BC_OK, S3_BC_UNCONFIGURED, S3_BC_PARTIAL,
    S3_BC_BAD_WIDTH, S3_BC_BAD_FRAME, S3_BC_REVERSE,
    S3_BC_UNMAPPED, S3_BC_ENCODER, S3_BC_CAPTURE_OVERFLOW,
    S3_BC_QUEUE_OVERFLOW, S3_BC_CONTEXT } S3BarcodeResult;
typedef struct
{
    bool configured, black_high, allow_reverse;
    int left_polarity, right_polarity;
    uint32_t narrow_min_counts, narrow_max_counts;
    float wide_ratio_min, wide_ratio_max, narrow_variation;
    uint32_t quiet_counts, rearm_counts, frame_timeout_ms, event_age_ms;
    S3NavAction mapping[26];
} S3BarcodeConfig;
typedef struct
{
    uint32_t left_count, right_count, captured_us, context, generation;
    bool high, encoder_valid, ambiguous;
} S3BarcodeEdge;
typedef struct
{
    uint32_t id, acquired_ms, captured_us, context, generation;
    uint32_t left_count, right_count;
    char symbol;
    S3NavAction action;
} S3NavEvent;
typedef struct
{
    uint32_t serviced_ms, sequence, generation, context;
    uint32_t accepted, rejected, discarded;
    unsigned queued;
    bool configured, encoder_valid;
    char symbol;
    S3BarcodeState state;
    S3BarcodeResult result;
} S3BarcodeSnapshot;
typedef struct
{
    S3BarcodeSnapshot status;
    S3BarcodeConfig config;
    S3BarcodeEdge previous, entry, exit;
    uint32_t widths[S3_BARCODE_WIDTHS];
    unsigned width_count;
    bool have_previous, have_white;
    uint32_t frame_started_us, next_id;
    S3NavEvent events[S3_NAV_CAPACITY];
    unsigned head, count;
} S3BarcodeDecoder;

bool s3_barcode_config_valid(const S3BarcodeConfig *config);
/* Counts are the sum of both signed-forward wheel travels: twice average
   quadrature counts. All count-based settings use these same units. */
bool s3_barcode_travel(const S3BarcodeConfig *config,
    const S3BarcodeEdge *from, const S3BarcodeEdge *to, uint32_t *counts);
S3BarcodeResult s3_code39_decode(const uint32_t widths[S3_BARCODE_WIDTHS],
    const S3BarcodeConfig *config, char *symbol);
void s3_barcode_decoder_init(S3BarcodeDecoder *decoder,
    const S3BarcodeConfig *config, uint32_t generation);
void s3_barcode_decoder_edge(S3BarcodeDecoder *decoder,
    const S3BarcodeEdge *edge);
void s3_barcode_decoder_poll(S3BarcodeDecoder *decoder,
    const S3BarcodeEdge *observation, uint32_t now_ms);
bool s3_barcode_decoder_take(S3BarcodeDecoder *decoder, S3NavEvent *event);
void s3_barcode_decoder_fault(S3BarcodeDecoder *decoder,
    S3BarcodeResult result);
const char *s3_barcode_state_name(S3BarcodeState state);
const char *s3_barcode_result_name(S3BarcodeResult result);
#endif
