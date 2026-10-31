/*
 *	encoder_math.h
 *	Kernel-free encoder arithmetic: quadrature decode and wrap-safe travel.
 *	The shared count wraps at ENC_COUNT_BITS and is never reset. Every
 *	consumer (Motion actions, barcode bar widths, hump travel, bypass legs)
 *	stores its own origin count and measures travel by modular subtraction.
 */

#ifndef ENCODER_MATH_H
#define ENCODER_MATH_H

#include <stdint.h>
#include "../common/robot_config.h"

/* Counts per millimetre of wheel travel (geometry is PROVISIONAL, D03). */
#define ENC_COUNTS_PER_MM (ENC_COUNTS_PER_REV / (3.14159265f * WHEEL_DIAMETER_MM))
#define ENC_QUAD_INVALID 2	/* skipped state or both channels changed */

/* Heading in degrees from signed wheel travel counts; positive turns left. */
static inline float enc_heading_deg(int32_t left, int32_t right)
{
    return ((float)right - (float)left) / ENC_COUNTS_PER_MM /
        TRACK_WIDTH_MM * 57.2957795f;
}

/* Quadrature step between two 2-bit levels (A*2+B): +1, -1, 0 (no change),
   or ENC_QUAD_INVALID. Gray sequence 00,01,11,10 counts up. */
static inline int enc_quad_step(unsigned before, unsigned after)
{
    static const int8_t table[16] = {0, 1, -1, 2, -1, 0, 2, 1,
                                     1, 2, 0, -1, 2, -1, 1, 0};
    return (before < 4u && after < 4u) ? table[before * 4u + after] : ENC_QUAD_INVALID;
}

/* count + delta, wrapped to ENC_COUNT_BITS. */
static inline uint32_t enc_advance(uint32_t count, int delta)
{
    return (count + (uint32_t)delta) & ENC_COUNT_MASK;
}

/* Forward travel since origin, 0..ENC_COUNT_MASK. */
static inline uint32_t enc_travel(uint32_t now, uint32_t origin)
{
    return (now - origin) & ENC_COUNT_MASK;
}

/* Signed travel since origin, in -2^(bits-1) .. 2^(bits-1)-1. */
static inline int32_t enc_travel_signed(uint32_t now, uint32_t origin)
{
    unsigned shift = 32u - ENC_COUNT_BITS;
    return (int32_t)(enc_travel(now, origin) << shift) >> shift;
}

/* Average displacements, never lifetime counts: wheels may diverge through
   repeated turns. Each wheel's displacement must stay below its half range. */
static inline float enc_mean_travel(const uint32_t count[2],
    const uint32_t origin[2])
{
    int64_t total = (int64_t)enc_travel_signed(count[0], origin[0]) +
        enc_travel_signed(count[1], origin[1]);
    return (float)total * 0.5f;
}

#endif /* ENCODER_MATH_H */
