/*
 *	terrain_pack.h
 *	Terrain word (writer: Terrain, one aligned word, single store). Layout is
 *	PROVISIONAL until #28/#53 sign off the S4 rows; scale is D06.
 *	  bits  0-6   last hump height result (7 bits)
 *	  bits  7-13  maximum hump height result (7 bits)
 *	  bits 14-21  hump count (8 bits, saturating)
 *	  bits 22-24  terrain state (3 bits)
 *	  bit  25     height valid (the last encounter produced a valid height)
 *	  bit  26     calibrated
 *	A result code equal to rd_invalid_code(HUMP_RESULT_BITS) means no valid
 *	height; the reported height is code * HUMP_MM_PER_COUNT millimetres.
 */

#ifndef TERRAIN_PACK_H
#define TERRAIN_PACK_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_data.h"
#include "../common/robot_config.h"

#define TERRAIN_STATE_BITS 3u

static inline uint32_t terrain_pack(uint32_t last_code, uint32_t max_code, uint32_t count, uint32_t state,
                                    bool height_valid, bool calibrated)
{
    uint32_t invalid = rd_invalid_code(HUMP_RESULT_BITS);
    uint32_t last = last_code > invalid ? invalid : last_code;
    uint32_t max = max_code > invalid ? invalid : max_code;
    return last | (max << 7) | ((count > 255u ? 255u : count) << 14) |
           ((state & rd_mask(TERRAIN_STATE_BITS)) << 22) | ((height_valid ? 1u : 0u) << 25) |
           ((calibrated ? 1u : 0u) << 26);
}

static inline uint32_t terrain_last_code(uint32_t w) { return rd_field_get(w, 0u, HUMP_RESULT_BITS); }
static inline uint32_t terrain_max_code(uint32_t w) { return rd_field_get(w, 7u, HUMP_RESULT_BITS); }
static inline uint32_t terrain_hump_count(uint32_t w) { return rd_field_get(w, 14u, 8u); }
static inline uint32_t terrain_state_code(uint32_t w) { return rd_field_get(w, 22u, TERRAIN_STATE_BITS); }
static inline bool terrain_height_valid(uint32_t w) { return rd_field_get(w, 25u, 1u) != 0u; }
static inline bool terrain_calibrated(uint32_t w) { return rd_field_get(w, 26u, 1u) != 0u; }

/* Millimetres for a result code; false for the invalid code. */
static inline bool terrain_code_to_mm(uint32_t code, uint32_t *mm)
{
    if (code >= rd_invalid_code(HUMP_RESULT_BITS)) return false;
    *mm = code * HUMP_MM_PER_COUNT;
    return true;
}

#endif /* TERRAIN_PACK_H */
