/*
 *	echo_range.h
 *	Kernel-free ultrasonic ranging helpers: pulse width to range, the packed
 *	front-range word and the no-echo counter.
 *
 *	Front-range word (writer: Obstacle). Layout is PROVISIONAL until the S5
 *	rows of DATA_REPRESENTATION are signed off (#28, #54):
 *	  bits  0-12  range in mm, 0..8190; 8191 is the invalid code
 *	  bit  13     valid (range is a real echo)
 *	  bits 16-31  low 16 bits of the sample time in kernel ms
 *	No echo is unknown, never clear space: valid is 0 and range is invalid.
 */

#ifndef ECHO_RANGE_H
#define ECHO_RANGE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/robot_data.h"

#define ECHO_RANGE_BITS 13u
#define ECHO_RANGE_INVALID 8191u

/* Range in mm for a pulse width; ECHO_RANGE_INVALID outside the usable window. */
static inline uint32_t echo_range_mm(uint32_t pulse_us)
{
    if (pulse_us < SONAR_MIN_PULSE_US || pulse_us >= SONAR_TIMEOUT_US) {
        return ECHO_RANGE_INVALID;
    }
    uint32_t mm = (pulse_us * SONAR_MM_PER_MS_X10) / 10000u;	/* us -> ms x10 scale */
    return mm >= ECHO_RANGE_INVALID ? ECHO_RANGE_INVALID : mm;
}

static inline uint32_t echo_pack(uint32_t range_mm, uint32_t sampled_ms)
{
    bool valid = range_mm < ECHO_RANGE_INVALID;
    return (valid ? range_mm : ECHO_RANGE_INVALID) | ((valid ? 1u : 0u) << 13) | ((sampled_ms & 0xFFFFu) << 16);
}

static inline bool echo_valid(uint32_t word) { return rd_field_get(word, 13u, 1u) != 0u; }
static inline uint32_t echo_range(uint32_t word) { return rd_field_get(word, 0u, ECHO_RANGE_BITS); }
static inline uint32_t echo_time16(uint32_t word) { return rd_field_get(word, 16u, 16u); }

/* Counts consecutive misses; true while the no-echo fault should be set. */
static inline bool echo_note(uint32_t *misses, bool got_echo)
{
    *misses = got_echo ? 0u : (*misses < 0xFFFFu ? *misses + 1u : *misses);
    return *misses >= SONAR_NO_ECHO_LIMIT;
}

/* Coherent acquisition evidence; packed low time bits remain display-only. */
typedef struct
{
    uint32_t word, sampled_ms, sequence;
} s5_range_sample_t;

static inline bool s5_range_sample_fresh(const s5_range_sample_t *sample,
    uint32_t now, uint32_t maximum_age_ms)
{
    return sample != 0 && sample->sequence != 0u && echo_valid(sample->word) &&
        (uint32_t)(now - sample->sampled_ms) <= maximum_age_ms;
}

#endif /* ECHO_RANGE_H */
