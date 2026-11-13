#include "mission_mapping.h"
#include "../common/robot_data.h"
#include <stdatomic.h>
#include <stddef.h>

/* Single-writer tickets serialize one request through result consumption.
   Words and tickets are release/acquire copies; no M0+ atomic RMW helpers. */
static rd_word_t request_word, reply_word;
static _Atomic uint32_t requested, replied;
static uint32_t submitted; /* I/O only */
static uint32_t consumed;  /* I/O only */
static uint32_t taken;               /* Mission only */
static uint8_t sequence;             /* Mission only */

enum { MAPPING_SEQUENCE_SHIFT = 21u, REPLY_RESULT_SHIFT = 8u,
    DIRECTION_SHIFT = 29u };

bool mission_mapping_submit(const mqtt_mapping_t *mapping)
{
    uint32_t packed;
    if (NULL == mapping || submitted != consumed ||
        !rd_mapping_pack(mapping->letters, &packed))
    {
        return false;
    }
    rd_word_publish(&request_word, packed |
        ((uint32_t)mapping->sequence << MAPPING_SEQUENCE_SHIFT));
    ++submitted;
    atomic_store_explicit(&requested, submitted, memory_order_release);
    return true;
}

bool mission_direction_submit(uint8_t seq, uint8_t action)
{
    static uint8_t last_sequence;
    static struct
    {
        uint8_t valid : 1;
    } duplicate;
    if (action < 1u || action > 4u || submitted != consumed ||
        (duplicate.valid && seq == last_sequence))
    {
        return false;
    }
    rd_word_publish(&request_word, ((uint32_t)seq << MAPPING_SEQUENCE_SHIFT) |
        ((uint32_t)action << DIRECTION_SHIFT));
    last_sequence = seq;
    duplicate.valid = 1u;
    ++submitted;
    atomic_store_explicit(&requested, submitted, memory_order_release);
    return true;
}

bool mission_mapping_take(mqtt_mapping_t *mapping, uint8_t *action)
{
    uint32_t ticket = atomic_load_explicit(&requested, memory_order_acquire);
    if (NULL == mapping || ticket == taken)
    {
        return false;
    }
    uint32_t word = rd_word_read(&request_word);
    for (unsigned i = 0u; i < RD_MAP_COUNT; ++i)
    {
        mapping->letters[i] = (uint8_t)rd_mapping_letter(word,
            (rd_map_action_t)i);
    }
    if (action != NULL)
    {
        *action = (uint8_t)(word >> DIRECTION_SHIFT);
    }
    mapping->sequence = (uint8_t)(word >> MAPPING_SEQUENCE_SHIFT);
    sequence = mapping->sequence;
    taken = ticket;
    return true;
}

void mission_mapping_reply(mqtt_ack_result_t result)
{
    if ((unsigned)result > MQTT_ACK_MALFORMED ||
        taken == atomic_load_explicit(&replied, memory_order_relaxed))
    {
        return;
    }
    rd_word_publish(&reply_word, sequence
        | ((uint32_t)result << REPLY_RESULT_SHIFT));
    atomic_store_explicit(&replied, taken, memory_order_release);
}

bool mission_mapping_poll(mqtt_ack_t *ack)
{
    uint32_t ticket = atomic_load_explicit(&replied, memory_order_acquire);
    if (NULL == ack || ticket == consumed)
    {
        return false;
    }
    uint32_t word = rd_word_read(&reply_word);
    *ack = (mqtt_ack_t){.sequence = (uint8_t)word,
        .result = (uint8_t)(word >> REPLY_RESULT_SHIFT)};
    consumed = ticket;
    return true;
}
