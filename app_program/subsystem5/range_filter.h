/* Fixed-storage repeated ranging; no clocks, waits, hardware or kernel calls. */
#ifndef RANGE_FILTER_H
#define RANGE_FILTER_H

#include <stdbool.h>
#include <stdint.h>
#include "echo_range.h"

#define S5_RANGE_GROUP_CAPACITY 5u

typedef struct
{
    uint16_t values[S5_RANGE_GROUP_CAPACITY];
    uint16_t attempts : 3;
    uint16_t accepted : 3;
    uint16_t requested : 3;
    uint16_t minimum : 3;
} s5_range_group_t;

/* Odd requested count 1..5; minimum valid count is a strict majority. */
bool s5_range_group_begin(s5_range_group_t *group, unsigned requested,
    unsigned minimum);
/* Exactly one attempt; false for an uninitialized/already-complete group. */
bool s5_range_group_push(s5_range_group_t *group, uint32_t range_mm);
bool s5_range_group_complete(const s5_range_group_t *group);
/* True only for a complete group with its minimum valid count. */
bool s5_range_group_median(const s5_range_group_t *group, uint16_t *range_mm);

_Static_assert(sizeof(s5_range_group_t) == 12u, "range group layout changed");

#endif /* RANGE_FILTER_H */
