#include "robot_data.h"
#include "robot_config.h"

/* One byte per owner; each byte has exactly one writer. */
static _Atomic uint8_t fault_byte[RD_OWNER_COUNT];

bool rd_mapping_letters_valid(const uint8_t letters[RD_MAP_COUNT])
{
    uint32_t seen = 0u;
    for (unsigned i = 0u; i < RD_MAP_COUNT; ++i) {
        if (letters[i] >= RD_MAP_LETTER_COUNT || (seen & (1u << letters[i])) != 0u) {
            return false;
        }
        seen |= 1u << letters[i];
    }
    return true;
}

bool rd_mapping_pack(const uint8_t letters[RD_MAP_COUNT], uint32_t *word)
{
    if (word == RD_NULL || !rd_mapping_letters_valid(letters)) {
        return false;
    }
    uint32_t packed = 0u;
    for (unsigned i = 0u; i < RD_MAP_COUNT; ++i) {
        packed |= (uint32_t)letters[i] << (i * RD_MAP_LETTER_BITS);
    }
    *word = packed | (1u << RD_MAP_MAPPED_SHIFT);
    return true;
}

bool rd_mapping_lookup(uint32_t word, uint8_t letter, rd_map_action_t *action)
{
    if (action == RD_NULL || !rd_mapping_is_mapped(word) || letter >= RD_MAP_LETTER_COUNT) {
        return false;
    }
    for (unsigned i = 0u; i < RD_MAP_COUNT; ++i) {
        if (rd_mapping_letter(word, (rd_map_action_t)i) == letter) {
            *action = (rd_map_action_t)i;
            return true;
        }
    }
    return false;
}

void rd_fault_set(rd_owner_t owner, uint8_t bits)
{
    if ((unsigned)owner < RD_OWNER_COUNT) {
        uint8_t now = atomic_load_explicit(&fault_byte[owner], memory_order_relaxed);
        atomic_store_explicit(&fault_byte[owner], (uint8_t)(now | bits), memory_order_release);
    }
}

void rd_fault_clear(rd_owner_t owner, uint8_t bits)
{
    if ((unsigned)owner < RD_OWNER_COUNT) {
        uint8_t now = atomic_load_explicit(&fault_byte[owner], memory_order_relaxed);
        atomic_store_explicit(&fault_byte[owner], (uint8_t)(now & (uint8_t)~bits), memory_order_release);
    }
}

uint8_t rd_fault_get(rd_owner_t owner)
{
    if ((unsigned)owner >= RD_OWNER_COUNT) {
        return 0u;
    }
    return atomic_load_explicit(&fault_byte[owner], memory_order_acquire);
}

static const uint8_t start_blocking[RD_OWNER_COUNT] = {
    [RD_OWNER_MOTION] = RD_FAULT_MOTOR_STALL | RD_FAULT_ENCODER,
    [RD_OWNER_LINE] = RD_FAULT_IR_INVALID,
    [RD_OWNER_TERRAIN] = RD_FAULT_IMPACT};

static const uint8_t run_stopping[RD_OWNER_COUNT] = {
    [RD_OWNER_MOTION] = RD_FAULT_MOTOR_STALL | RD_FAULT_ENCODER,
    [RD_OWNER_TERRAIN] = RD_FAULT_IMPACT};

static bool any_fault(const uint8_t mask[RD_OWNER_COUNT])
{
    for (unsigned i = 0u; i < RD_OWNER_COUNT; ++i) {
        if ((rd_fault_get((rd_owner_t)i) & mask[i]) != 0u) {
            return true;
        }
    }
    return false;
}

bool rd_fault_start_blocked(void)
{
    if (APP_NO_GUARDS) return false;	/* bench build: faults are reported only */
    return any_fault(start_blocking);
}

bool rd_fault_stop_pending(void)
{
    if (APP_NO_GUARDS) return false;	/* bench build: GP21 is the only stop */
    return any_fault(run_stopping);
}
