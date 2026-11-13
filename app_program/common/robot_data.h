/*
 *  robot_data.h
 *  Packed shared-data layer. Three mechanisms only (ARCHITECTURE, Shared-data
 *  rules):
 *    1. Word snapshot: at most 32 bits packed into one aligned uint32_t.
 *       The single writer composes the whole word in a register and publishes
 *       it with one store (rd_word_publish); readers take one load
 *       (rd_word_read) and decode with accessors. Aligned 32-bit loads and
 *       stores are single-copy atomic on RP2040 across both cores.
 *    2. Mutex copy: snapshots above 32 bits, waits at most RD_SNAPSHOT_LOCK_MS.
 *    3. Fault bytes: one byte per owner; an owner sets and clears only the
 *       bits of its own byte; Mission and Safety read them through masks.
 *  Never publish a reference into another subsystem's storage, and never
 *  read-modify-write a shared word: build the new word, then publish it.
 *
 *  This header is host-testable (no kernel includes).
 */

#ifndef ROBOT_DATA_H
#define ROBOT_DATA_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

/* No <stddef.h>: the kernel headers define their own size_t. */
#define RD_NULL ((void *)0)

/* --- Word snapshot ---------------------------------------------------- */

/* One storage unit, exactly one writer (named where the unit is defined). */
typedef _Atomic uint32_t rd_word_t;

static inline void rd_word_publish(rd_word_t *slot, uint32_t word)
{
    atomic_store_explicit(slot, word, memory_order_release);
}

static inline uint32_t rd_word_read(const rd_word_t *slot)
{
    return atomic_load_explicit(slot, memory_order_acquire);
}

/* --- Field accessors (width 1..31) ------------------------------------- */

static inline uint32_t rd_mask(unsigned width)
{
    return (1u << width) - 1u;
}

/* Signed values live in unsigned bit-fields (BARR-C:2018 5.3.a). rd_sext
   sign-extends a width-bit field on read; rd_swrap masks a value to the
   width on write. Callers clamp first, so nothing is lost. */
static inline int32_t rd_sext(uint32_t raw, unsigned bits)
{
    uint32_t sign = 1u << (bits - 1u);
    return (int32_t)(raw ^ sign) - (int32_t)sign;
}

static inline uint32_t rd_swrap(int32_t value, unsigned bits)
{
    return (uint32_t)value & rd_mask(bits);
}

static inline uint32_t rd_field_get(uint32_t word, unsigned shift,
                                    unsigned width)
{
    return (word >> shift) & rd_mask(width);
}

/* Replaces the field with value; a value that does not fit is rejected and
   the word is returned unchanged (*ok false) rather than silently truncated. */
static inline uint32_t rd_field_set(uint32_t word, unsigned shift,
                                    unsigned width,
                                    uint32_t value, bool *ok)
{
    bool fits = value <= rd_mask(width);
    if (ok != RD_NULL)
    {
        *ok = fits;
    }
    if (!fits)
    {
        return word;
    }
    return (word & ~(rd_mask(width) << shift)) | (value << shift);
}

/* Saturating narrowing of a non-negative quantity to `width` bits, reserving
   the all-ones code as the invalid code: valid values are 0..2^width-2. */
static inline uint32_t rd_sat_valid(uint32_t value, unsigned width)
{
    uint32_t top = rd_mask(width) - 1u;
    return value > top ? top : value;
}

static inline uint32_t rd_invalid_code(unsigned width)
{
    return rd_mask(width);
}

/* --- Barcode mapping word (writer: Mission) ----------------------------
   Four 5-bit letter indices (A=0..Z=25) plus a MAPPED flag. A zero word is
   UNMAPPED (boot state), whatever the letter fields hold. */

typedef enum
{
    RD_MAP_LEFT,
    RD_MAP_RIGHT,
    RD_MAP_STRAIGHT,
    RD_MAP_UTURN,
    RD_MAP_COUNT
} rd_map_action_t;

#define RD_MAP_LETTER_BITS  5u
#define RD_MAP_LETTER_COUNT 26u
#define RD_MAP_MAPPED_SHIFT (RD_MAP_COUNT * RD_MAP_LETTER_BITS)

/* True when all four letters are A..Z and pairwise distinct. */
bool rd_mapping_letters_valid(const uint8_t letters[RD_MAP_COUNT]);

/* Packs a mapping; false (and *word untouched) if the letters are invalid. */
bool rd_mapping_pack(const uint8_t letters[RD_MAP_COUNT], uint32_t *word);

static inline bool rd_mapping_is_mapped(uint32_t word)
{
    return rd_field_get(word, RD_MAP_MAPPED_SHIFT, 1u) != 0u;
}

static inline uint8_t rd_mapping_letter(uint32_t word, rd_map_action_t action)
{
    return (uint8_t)rd_field_get(word,
                                 (unsigned)action * RD_MAP_LETTER_BITS,
                                 RD_MAP_LETTER_BITS);
}

/* Action for a decoded letter; false if unmapped or the letter is unused. */
bool rd_mapping_lookup(uint32_t word, uint8_t letter, rd_map_action_t *action);

/* --- Motion feedback word (writer: Motion) ------------------------------
   2-bit status plus action ID. Action ID width is PROVISIONAL until the
   Motion rows of DATA_REPRESENTATION are signed off. */

typedef enum
{
    RD_MOTION_IDLE,
    RD_MOTION_RUNNING,
    RD_MOTION_DONE,
    RD_MOTION_FAILED
} rd_motion_status_t;

#define RD_MOTION_STATUS_BITS   2u
#define RD_MOTION_ACTION_BITS   6u  /* PROVISIONAL (#28) */

static inline uint32_t rd_motion_feedback_pack(rd_motion_status_t status,
                                               uint32_t action_id,
                                               bool *ok)
{
    bool fits_id;
    uint32_t word = rd_field_set(0u, RD_MOTION_STATUS_BITS,
                                 RD_MOTION_ACTION_BITS, action_id,
                                 &fits_id);
    bool fits_status = (unsigned)status <= rd_mask(RD_MOTION_STATUS_BITS);
    if (ok != RD_NULL)
    {
        *ok = fits_id && fits_status;
    }
    return (fits_id && fits_status) ? (word | (uint32_t)status) : 0u;
}

static inline rd_motion_status_t rd_motion_feedback_status(uint32_t word)
{
    return (rd_motion_status_t)rd_field_get(word, 0u, RD_MOTION_STATUS_BITS);
}

static inline uint32_t rd_motion_feedback_action(uint32_t word)
{
    return rd_field_get(word, RD_MOTION_STATUS_BITS, RD_MOTION_ACTION_BITS);
}

/* --- Fault bytes --------------------------------------------------------
   Every bit is reported over MQTT. Two masks decide what a bit does:
   start-blocking bits keep Mission in READY; run-stopping bits make Safety
   latch a stop. A bit in neither is report-only (missing or unreliable data
   degrades a measurement but is not a hazard). */

typedef enum
{
    RD_OWNER_MOTION, /* Motion task */
    RD_OWNER_LINE, /* line_barcode task: IR and barcode capture */
    RD_OWNER_TERRAIN, /* Terrain */
    RD_OWNER_OBSTACLE, /* Obstacle */
    RD_OWNER_MISSION, /* Mission task */
    RD_OWNER_NETWORK, /* kernel task, core 0 */
    RD_OWNER_COUNT
} rd_owner_t;

/* Bit positions await owner confirmation (#113); status in docs/architecture.md
   Fault bitmask. scripts/mqtt_client.py --self-test checks its decoder table
   against this enum. */
enum
{
    RD_FAULT_MOTOR_STALL = 1u << 0, /* start-blocking, run-stopping */
    RD_FAULT_ENCODER = 1u << 1,     /* start-blocking, run-stopping */

    RD_FAULT_IR_INVALID = 1u << 0,  /* start-blocking */
    /* Overflow is a per-run condition, cleared when the scanner stops, so it
       is reported during the run and never blocks the next start. */
    RD_FAULT_BARCODE_CAPTURE_OVERFLOW = 1u << 1,    /* report only */
    RD_FAULT_BARCODE_EVENT_OVERFLOW = 1u << 2,  /* report only */

    RD_FAULT_IMU = 1u << 0,     /* report only */
    RD_FAULT_CALIBRATION_FAILED = 1u << 1,  /* start-blocking */
    RD_FAULT_LIFTED = 1u << 2,  /* start-blocking, run-stopping */

    RD_FAULT_NO_ECHO = 1u << 0,     /* report only: open space gives no echo */

    RD_FAULT_LINE_LOST = 1u << 0,   /* report only: Mission stops itself */

    /* report only: networking never starts or stops */
    RD_FAULT_WIFI_LOST = 1u << 0,
    RD_FAULT_BROKER_LOST = 1u << 1  /* report only */
};

/* Defined bits per owner byte, rd_owner_t order. Telemetry packs exactly
   these into one 12-bit fault word (MQTT schema v3). */
#define RD_FAULT_WIDTHS { 2u, 3u, 3u, 1u, 1u, 2u }
#define RD_FAULT_BITS 12u
_Static_assert((RD_FAULT_ENCODER < (1u << 2)) &&
    (RD_FAULT_BARCODE_EVENT_OVERFLOW < (1u << 3)) &&
    (RD_FAULT_LIFTED < (1u << 3)) && (RD_FAULT_NO_ECHO < (1u << 1)) &&
    (RD_FAULT_LINE_LOST < (1u << 1)) && (RD_FAULT_BROKER_LOST < (1u << 2)),
    "every fault bit fits its owner's width");

/* Only the owner calls set/clear for its own byte (single writer, so the
   load-modify-store cannot race). Any task may read. */
void rd_fault_set(rd_owner_t owner, uint8_t bits);
void rd_fault_clear(rd_owner_t owner, uint8_t bits);
uint8_t rd_fault_get(rd_owner_t owner);

/* True when a start-blocking bit is set: Mission refuses a start. */
bool rd_fault_start_blocked(void);

/* True when a run-stopping bit is set: Safety latches a stop. */
bool rd_fault_stop_pending(void);

#endif /* ROBOT_DATA_H */
