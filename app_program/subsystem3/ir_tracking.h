#ifndef IR_TRACKING_H
#define IR_TRACKING_H

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
} S3IrSnapshot;

S3IrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now);
/* Recompute age at read time without changing sequence or acquisition time. */
void ir_snapshot_refresh(S3IrSnapshot *snapshot, uint32_t now);

#if APP_TEST
const char *ir_tracking_line_name(IrLine line);
#endif

#endif
