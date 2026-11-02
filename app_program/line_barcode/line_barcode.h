/* Line and barcode public interface: line classification, barcode decoding and task API. */
#ifndef LINE_BARCODE_H
#define LINE_BARCODE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

/* Digital raw readings are GPIO levels 0 or 1. Legacy 12-bit packing and
   snapshot fields remain for telemetry/API compatibility. */

/* Base-2 scaled integers (ADR 0001): a 0..1 fraction is stored as 0..127 and
   a signed -1..+1 value as -127..+127 (int8_t). Divide by IR_SCALE_ONE only
   at a float boundary (Mission building the S2 motion command). */
#define IR_SCALE_ONE 127

typedef struct
{
    uint16_t left;
    uint16_t right;
} IrRaw;

/* A raw pair and its valid flag in one word: left bits 0-11, right bits
   12-23, valid bit 24. Written with one aligned 32-bit store, so no reader
   sees a left from one sample beside a right from another. Readings above
   IR_ADC_MAX (only possible with valid clear) saturate to IR_ADC_MAX. */
typedef uint32_t IrRawWord;

#define IR_RAW_BITS 12u
#define IR_RAW_MASK ((1u << IR_RAW_BITS) - 1u)
#define IR_RAW_VALID_BIT (2u * IR_RAW_BITS)
_Static_assert(IR_ADC_MAX == IR_RAW_MASK, "raw IR field width must match the ADC");

static inline IrRawWord ir_raw_word(IrRaw raw, bool valid)
{
    uint32_t left = raw.left > IR_ADC_MAX ? IR_ADC_MAX : raw.left;
    uint32_t right = raw.right > IR_ADC_MAX ? IR_ADC_MAX : raw.right;
    return left | right << IR_RAW_BITS | (valid ? 1u : 0u) << IR_RAW_VALID_BIT;
}

static inline IrRaw ir_raw_word_raw(IrRawWord word)
{
    return (IrRaw){(uint16_t)(word & IR_RAW_MASK), (uint16_t)(word >> IR_RAW_BITS & IR_RAW_MASK)};
}

static inline bool ir_raw_word_valid(IrRawWord word)
{
    return (word >> IR_RAW_VALID_BIT & 1u) != 0u;
}

typedef enum
{
    IR_LINE_UNKNOWN,
    IR_LINE_VISIBLE,
    IR_LINE_LOST,
    IR_LINE_BROAD_BLACK
} IrLine;

typedef struct
{
    bool valid;
    uint8_t left_fraction;	/* digital: 0 white, IR_SCALE_ONE black */
    uint8_t right_fraction;
    int8_t error;		/* right - left, -IR_SCALE_ONE..+IR_SCALE_ONE */
} IrReading;

typedef struct
{
    IrLine line;
    IrReading reading;
    IrRawWord raw;		/* latest pair; valid flag = sample valid */
    bool have_sample;
    uint32_t sample_ms;
} IrTracker;

/* Valid guidance is fresh digital VISIBLE data, never motion permission.
   error is -127 left black, +127 right black, 0 both white.
   Both white is forward; both black is a junction, without guidance.
   Mission maps left black to negative steering, right black to positive,
   matching the mounted robot's observed direction. */
typedef struct
{
    bool valid;
    IrLine line;
    int8_t error;	/* -IR_SCALE_ONE..+IR_SCALE_ONE */
    uint32_t sampled_ms;
} IrTrackingOutput;

void ir_tracking_init(IrTracker *tracker);
/* now is paired acquisition completion time, not publication time. */
void ir_tracking_sample(IrTracker *tracker, IrRaw raw, bool acquired,
                        uint32_t now);
IrReading ir_tracking_reading(IrRaw raw);
IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now);
/* Portable data for S1 and future Mission consumers; no wire format implied.
   Invalid fraction/error fields are zero; use validity flags. */
typedef struct
{
    uint32_t sequence;
    uint32_t sampled_ms;
    uint32_t sample_age_ms;
    IrRawWord raw;		/* valid flag = acquisition valid */
    uint8_t left_fraction;	/* 0..IR_SCALE_ONE */
    uint8_t right_fraction;
    int8_t error;		/* right - left, see IrTrackingOutput */
    IrLine line;
    bool have_sample;
    bool sample_fresh;
    bool normalized_valid;
    bool guidance_valid;
} LineIrSnapshot;

LineIrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now);
/* Recompute age at read time without changing sequence or acquisition time. */
void ir_snapshot_refresh(LineIrSnapshot *snapshot, uint32_t now);

#if APP_TEST
const char *ir_tracking_line_name(IrLine line);
#endif

/* Called only by S3; shared timebase must already be initialized. */
bool drv_ir_tracking_init(void);
/* False leaves *raw unchanged. No connectivity inference from digital levels. */
bool drv_ir_tracking_read(IrRaw *raw);

/*
 *	Kernel-free barcode types, Code 39 decoder and frame assembler: turns edges
 *	(level plus the two wheel encoder counts at the edge) into element widths
 *	measured in encoder travel, waits for a quiet zone, and decodes 29 elements.
 */

#define BARCODE_ELEMENTS 29u	/* *X*: three 9-element characters and two gaps */
#define BARCODE_ALPHABET 26u

/* Encoder counts as S3 stores them: the shared ENC_COUNT_BITS width, wrapped.
   Travel between two counts uses modular subtraction (motion.h). */
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

/*
 *	GP16 edge capture. The dispatcher (core 1) calls the edge handler, which
 *	pushes {both encoder counts, level} into a single-producer /
 *	single-consumer ring and returns. The task drains the ring.
 */

typedef struct
{
    barcode_count_t left_count, right_count;	/* wrapped at ENC_COUNT_BITS */
    bool high;	/* GP16 level after the edge, or at enable-time reference */
} barcode_edge_t;

/* Core-1 task only: configure GP16 and claim it on the GPIO dispatcher. The
   edge IRQ stays disabled until drv_barcode_enable(true). */
bool drv_barcode_init(void);
/* Core-1 task only. Enabling clears old edges and queues the current level
   and coherent wheel counts as the quiet-zone reference for this session.
   Overflow/order counters remain cumulative. */
void drv_barcode_enable(bool enabled);
/* Task side: pop one edge. False when empty. */
bool drv_barcode_take(barcode_edge_t *edge);
/* ISR-written counters (single writer: the handler). Compare with a stored
   value; a change means the ring overflowed / both edges arrived together. */
uint32_t drv_barcode_overflow_count(void);
uint32_t drv_barcode_order_lost_count(void);

/*
 *	Barcode scanner owned by the S3 task (core 1): edge ring -> frame
 *	assembler -> Code 39 A-Z decoder -> mapped action -> small event queue.
 *	Boots UNMAPPED and disabled; start is refused until a mapping of four
 *	distinct letters (left, right, straight, U-turn) is installed.
 */

/* Core-1 task only: claim GP16 on the dispatcher. False on failure. */
bool barcode_init(void);

/* Install four distinct letters A..Z, indexed by rd_map_action_t. Refused
   while scanning or if the letters are invalid; true on success. */
bool barcode_set_mapping(const uint8_t letters[4]);

/* True once a valid four-letter mapping is installed. */
bool barcode_mapped(void);

/* Mission only. Start is refused (false) until a mapping is installed; it
   drops stale events and asks the S3 task to rearm. The S3 task applies
   start/stop at its next release, resetting the frame and the barcode
   overflow bits. */
bool barcode_start(void);
void barcode_stop(void);

/* S3 task, once per release: apply start/stop, drain the ring with a fixed
   edge budget, assemble and decode frames, queue events, map overflow to the
   Line fault byte. cancel (GP21 held) disables scanning while held. */
void barcode_service(bool cancel);

/* Exactly one consumer removes events. False when empty. */
bool barcode_pop(barcode_event_t *event);

/* S3-task-only optional diagnostic ring. Never called from ISR/Mission. */
typedef struct {
    uint16_t left, right, width;
    uint16_t result : 3;
    uint16_t orientation : 1;
    uint16_t letter : 5; /* 0..25, 31 unavailable */
    uint16_t action : 3;
} barcode_diagnostic_t;
bool barcode_diagnostic_pop(barcode_diagnostic_t *record, unsigned *dropped);

ER line_barcode_init(void);
void line_barcode_step(uint32_t now_ms);
/* 
 * Coherent copy; waits at most 2 ms for the publisher. False means null,
 * uninitialized, or lock/unlock error;
 * Ignore the destination on failure; normalized zero is not validity.
 * This API is for task context, never an ISR.
*/
bool line_ir_copy(LineIrSnapshot *snapshot);
bool line_ir_try_copy(LineIrSnapshot *snapshot);

#endif /* LINE_BARCODE_H */
