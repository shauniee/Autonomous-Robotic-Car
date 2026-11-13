/* Line and barcode public interface: line classification, barcode decoding
   and task API. */
#ifndef LINE_BARCODE_H
#define LINE_BARCODE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

/* Digital raw readings are GPIO levels 0 or 1. */

/* Base-2 scaled integers (docs/architecture.md): a 0..1 fraction is stored as
   0..127 and a signed -1..+1 value as -127..+127 (int8_t). Divide by
   IR_SCALE_ONE only at a float boundary (Mission building the motion
   command). */
#define IR_SCALE_ONE 127

/* Digital level 0/1 per sensor; 2..3 are driver errors read as invalid. */
typedef struct
{
    uint8_t left : 2;
    uint8_t right : 2;
} IrRaw_t;

/* A raw pair and its valid flag in one word: left bit 0, right bit 1,
   valid bit 2. Written with one byte store, so no reader
   sees a left from one sample beside a right from another. Readings above
   1 (only possible with valid clear) saturate to 1. */
typedef uint8_t IrRawWord_t;

#define IR_RAW_BITS 1u
#define IR_RAW_MASK ((1u << IR_RAW_BITS) - 1u)
#define IR_RAW_VALID_BIT (2u * IR_RAW_BITS)

static inline IrRawWord_t ir_raw_word(IrRaw_t raw, bool valid)
{
    uint32_t left = raw.left > IR_RAW_MASK ? IR_RAW_MASK : raw.left;
    uint32_t right = raw.right > IR_RAW_MASK ? IR_RAW_MASK : raw.right;
    return (IrRawWord_t)(left | right << IR_RAW_BITS |
        (valid ? 1u : 0u) << IR_RAW_VALID_BIT);
}

static inline IrRaw_t ir_raw_word_raw(IrRawWord_t word)
{
    return (IrRaw_t){(uint8_t)(word & IR_RAW_MASK),
        (uint8_t)(word >> IR_RAW_BITS & IR_RAW_MASK)};
}

static inline bool ir_raw_word_valid(IrRawWord_t word)
{
    return (word >> IR_RAW_VALID_BIT & 1u) != 0u;
}

typedef enum
{
    IR_LINE_UNKNOWN,
    IR_LINE_VISIBLE,
    IR_LINE_LOST,
    IR_LINE_BROAD_BLACK
} IrLine_t;

typedef struct
{
    int8_t error; /* right - left, -IR_SCALE_ONE..+IR_SCALE_ONE */
    uint8_t left_fraction : 7; /* digital: 0 white, IR_SCALE_ONE black */
    uint8_t right_fraction : 7;
    bool valid : 1;
} IrReading_t;

_Static_assert(IR_SCALE_ONE < 128, "IR fractions fit 7 bits");

typedef struct
{
    uint32_t sample_ms;
    IrRawWord_t raw; /* latest pair; valid flag = sample valid */
    uint8_t line : 2; /* IrLine_t */
    uint8_t have_sample : 1;
} IrTracker_t;

/* Valid guidance is fresh digital VISIBLE data, never motion permission.
   error is -127 left black, +127 right black, 0 both white.
   Both white is forward; both black is a junction, without guidance.
   Mission maps left black to negative steering, right black to positive,
   matching the mounted robot's observed direction. */
typedef struct
{
    uint32_t sampled_ms;
    int8_t error; /* -IR_SCALE_ONE..+IR_SCALE_ONE */
    uint8_t line : 2; /* IrLine_t */
    bool valid : 1;
} IrTrackingOutput_t;

void ir_tracking_init(IrTracker_t *tracker);
/* now is paired acquisition completion time, not publication time. */
void ir_tracking_sample(IrTracker_t *tracker, IrRaw_t raw, bool acquired,
                        uint32_t now);
IrReading_t ir_tracking_reading(IrRaw_t raw);
IrTrackingOutput_t ir_tracking_output(const IrTracker_t *tracker, uint32_t now);
/* Portable data for Comms and future Mission consumers; no wire format implied.
   Invalid black flags are zero; use validity flags. */
typedef struct
{
    uint32_t sampled_ms;
    IrRawWord_t raw; /* valid flag = acquisition valid */
    uint8_t line : 2; /* IrLine_t */
    uint8_t left_black : 1;
    uint8_t right_black : 1;
    uint8_t have_sample : 1;
    uint8_t sample_fresh : 1;
    uint8_t normalized_valid : 1;
    uint8_t guidance_valid : 1;
} LineIrSnapshot_t;

/* right - left: -IR_SCALE_ONE left black, +IR_SCALE_ONE right black. */
static inline int8_t ir_snapshot_error(const LineIrSnapshot_t *snapshot)
{
    return (int8_t)(((int)snapshot->right_black -
        (int)snapshot->left_black) * IR_SCALE_ONE);
}

LineIrSnapshot_t ir_snapshot_build(const IrTracker_t *tracker, uint32_t now);
/* Recompute age at read time without changing sequence or acquisition time. */
void ir_snapshot_refresh(LineIrSnapshot_t *snapshot, uint32_t now);

#if APP_TEST
const char *ir_tracking_line_name(IrLine_t line);
#endif

/* Called only by line_barcode; shared timebase must already be initialized. */
bool drv_ir_tracking_init(void);
/* False leaves *raw unchanged. No connectivity inference from digital
   levels. */
bool drv_ir_tracking_read(IrRaw_t *raw);

/*
 *    Kernel-free barcode types, Code 39 decoder and frame assembler: turns
 *    edges (level plus the two wheel encoder counts at the edge) into element
 *    widths measured in encoder travel, and tries the last 25 elements as a
 *    symbol.
 */

/* end of the first guard, gap, letter, gap, last guard */
#define BARCODE_ELEMENTS 25u
#define BARCODE_ALPHABET 26u

/* Encoder counts as line_barcode stores them: the shared ENC_COUNT_BITS width,
   wrapped. Travel between two counts uses modular subtraction (motion.h). */
typedef uint16_t barcode_count_t;
_Static_assert(ENC_COUNT_BITS <= 16u,
    "barcode_count_t narrower than ENC_COUNT_BITS");
/* The longest window (25 elements at the longest width) spans less than half
   the count range, so modular subtraction inside a frame cannot alias. A wider
   or narrower ENC_COUNT_BITS (D03) re-checks this bound. */
_Static_assert((uint32_t)BARCODE_ELEMENTS * BARCODE_MAX_ELEMENT_COUNTS <
    ENC_COUNT_MASK / 2u,
    "longest window must span less than half the encoder count range");

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
    unsigned letter = symbol >= 'A' && symbol <= 'Z' ?
        (unsigned)(symbol - 'A') : BARCODE_NO_LETTER;
    unsigned a = (unsigned)action <= (unsigned)BARCODE_U_TURN ?
        (unsigned)action : (unsigned)BARCODE_UNMAPPED;
    e->packed = (uint16_t)(letter | a << BARCODE_EVENT_ACTION_SHIFT |
        (BARCODE_REVERSE == orientation ? 1u : 0u) <<
        BARCODE_EVENT_ORIENTATION_SHIFT);
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

static inline barcode_orientation_t barcode_event_orientation(
    const barcode_event_t *e)
{
    return (e->packed >> BARCODE_EVENT_ORIENTATION_SHIFT & 1u) ?
        BARCODE_REVERSE : BARCODE_FORWARD;
}

/* Pure, bounded decoder of the last 25 elements of *X*, in travel order.
   BARCODE_NO_RESULT if these widths are not a symbol. Failure leaves
   destinations unchanged. */
barcode_result_t barcode_decode(const uint16_t widths[BARCODE_ELEMENTS],
    char *symbol, barcode_orientation_t *orientation);

/* Widths are encoder counts of sensor travel (the most recent runs, oldest
   first); every black run tries the last 25 as a symbol, so no quiet
   zone is needed and any false start clears itself. */
/* Stored widths never exceed BARCODE_MAX_ELEMENT_COUNTS, so 9 bits each;
   carry is at most one stored width plus one chatter run. */
#define BARCODE_WIDTH_BITS 9u
#define BARCODE_WIDTH_BYTES \
    ((BARCODE_ELEMENTS * BARCODE_WIDTH_BITS + 7u) / 8u)
_Static_assert(BARCODE_MAX_ELEMENT_COUNTS + BARCODE_MIN_ELEMENT_COUNTS <=
    (1u << BARCODE_WIDTH_BITS), "barcode widths and carry fit 9 bits");

typedef struct
{
    /* 25 x 9-bit widths, LSB first; use barcode_frame_width(). */
    uint8_t widths[BARCODE_WIDTH_BYTES];
    barcode_count_t prev_left, prev_right;
    uint16_t carry : 9; /* chatter travel owed to the run after it */
    uint16_t count : 5; /* 0..BARCODE_ELEMENTS */
    uint16_t have_edge : 1;
    uint16_t prev_black : 1;
    uint16_t last_width : 15; /* element this edge completed, 0 none */
} barcode_frame_t;

void barcode_frame_reset(barcode_frame_t *f);
/* Stored width at index, oldest first; index < f->count. */
uint16_t barcode_frame_width(const barcode_frame_t *f, unsigned index);
/* True if the sensor travelled too little between two edges for a real
   element. */
bool barcode_run_is_chatter(barcode_count_t from_left,
    barcode_count_t from_right, barcode_count_t left_count,
    barcode_count_t right_count);

/* Feed one edge. `black` is the level after the edge. Returns BARCODE_DECODED
   (symbol/orientation set) when a symbol completes, BARCODE_NO_RESULT while
   accumulating, otherwise why the runs so far were dropped or not a letter. */
barcode_result_t barcode_frame_edge(barcode_frame_t *f, bool black,
    barcode_count_t left_count, barcode_count_t right_count, char *symbol,
    barcode_orientation_t *orientation);

/* Action for a decoded letter under a mapping word (BARCODE_UNMAPPED when
   the word is unmapped or the letter is not one of the four). */
barcode_action_t barcode_action_for(uint32_t mapping_word, char symbol);

/*
 *  GP27 edge capture. The dispatcher (core 1) calls the edge handler, which
 *    pushes {both encoder counts, level} into a single-producer /
 *    single-consumer ring and returns. The task drains the ring.
 */

typedef struct
{
    barcode_count_t left_count;
    barcode_count_t right_count; /* wrapped at ENC_COUNT_BITS */
    bool high : 1; /* GP27 level after the edge, or enable-time reference */
} barcode_edge_t;

/* Core-1 task only: configure GP27 and claim it on the GPIO dispatcher. The
   edge IRQ stays disabled until drv_barcode_enable(true). */
bool drv_barcode_init(void);
/* Core-1 task only. Enabling clears old edges and queues the current level
   and coherent wheel counts as the starting reference for this session.
   The overflow counter remains cumulative. */
void drv_barcode_enable(bool enabled);
/* Task side: pop one edge. False when empty. */
bool drv_barcode_take(barcode_edge_t *edge);
/* ISR-written counter (single writer: the handler). Compare with a stored
   value; a change means the ring overflowed. */
uint32_t drv_barcode_overflow_count(void);
/* Every edge the handler has seen, for diagnostics. Wraps. */
uint32_t drv_barcode_edge_count(void);

/*
 * Barcode scanner owned by the line_barcode task (core 1): edge ring -> frame
 * assembler -> Code 39 A-Z decoder -> mapped action -> small event queue.
 * Boots UNMAPPED and disabled; start is refused until a mapping of four
 * distinct letters (left, right, straight, U-turn) is installed.
 */

/* Core-1 task only: claim GP27 on the dispatcher. False on failure. */
bool barcode_init(void);

/* Install four distinct letters A..Z, indexed by rd_map_action_t. Refused
   while scanning or if the letters are invalid; true on success. */
bool barcode_set_mapping(const uint8_t letters[4]);

/* True once a valid four-letter mapping is installed. */
bool barcode_mapped(void);

/* Mission only. Start is refused (false) until a mapping is installed; it
   drops stale events and asks the line_barcode task to rearm. The line_barcode
   task applies start/stop at its next release, resetting the frame and the
   barcode overflow bits. */
bool barcode_start(void);
void barcode_stop(void);

/* line_barcode task, once per release: apply start/stop, drain the ring with a
   fixed edge budget, assemble and decode frames, queue events, map overflow to
   the Line fault byte. cancel (GP21 held) disables scanning while held. */
void barcode_service(bool cancel);

/* Exactly one consumer removes events. False when empty. */
bool barcode_pop(barcode_event_t *event);

/* Task side: one raw run-width trace frame for the laptop. 0 when empty. */
unsigned barcode_trace_take(uint8_t *out, unsigned capacity);

/* line_barcode-task-only optional diagnostic ring. Never called from
   ISR/Mission. */
typedef struct
{
    uint16_t left;
    uint16_t right;
    uint16_t width;
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
bool line_ir_copy(LineIrSnapshot_t *snapshot);
bool line_ir_try_copy(LineIrSnapshot_t *snapshot);

#endif /* LINE_BARCODE_H */
