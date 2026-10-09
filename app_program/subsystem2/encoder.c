#include "encoder.h"
#include "../gpio_capture.h"
#include <stdatomic.h>

#define ENCODER_MASK ((1u << 0) | (1u << 1) | (1u << 7) | (1u << 28))
static atomic_uint version, left_value, right_value, error_value;
static atomic_uint sequence_value, time_value;
static atomic_bool ready;
static unsigned left_phase, right_phase;
static uint32_t left_count, right_count, errors, sequence;

static unsigned phase(uint32_t levels, unsigned a, unsigned b)
{
    return ((levels >> a) & 1u) * 2u + ((levels >> b) & 1u);
}

static void encoder_isr(uint32_t levels, uint32_t changed,
    uint32_t ambiguous, uint32_t timestamp_us)
{
    (void)changed;
    unsigned left = phase(levels, 28u, 7u);
    unsigned right = phase(levels, 1u, 0u);
    int dl = s2_encoder_transition(left_phase, left);
    int dr = s2_encoder_transition(right_phase, right);
    left_phase = left;
    right_phase = right;
    if ((dl == 2) || (dr == 2) || (ambiguous != 0u))
    {
        ++errors;
    }
    else
    {
        left_count += (uint32_t)dl;
        right_count += (uint32_t)dr;
    }
    ++sequence;
    atomic_store_explicit(&version, sequence * 2u + 1u,
        memory_order_seq_cst);
    atomic_store_explicit(&left_value, left_count, memory_order_seq_cst);
    atomic_store_explicit(&right_value, right_count, memory_order_seq_cst);
    atomic_store_explicit(&error_value, errors, memory_order_seq_cst);
    atomic_store_explicit(&sequence_value, sequence, memory_order_seq_cst);
    atomic_store_explicit(&time_value, timestamp_us, memory_order_seq_cst);
    atomic_store_explicit(&version, sequence * 2u + 2u,
        memory_order_seq_cst);
}

bool s2_encoder_init(void)
{
    const unsigned pins[] = {0u, 1u, 7u, 28u};
    for (unsigned i = 0u; i < 4u; ++i)
    {
        if (gpio_set_pin(pins[i], GPIO_MODE_IN) < E_OK)
        {
            return false;
        }
    }
    uint32_t levels = in_w(0xd0000004u);
    left_phase = phase(levels, 28u, 7u);
    right_phase = phase(levels, 1u, 0u);
    return gpio_capture_register(ENCODER_MASK, encoder_isr) >= E_OK;
}

void s2_encoder_enable_context(void)
{
    atomic_store_explicit(&ready, true, memory_order_release);
}

bool s2_encoder_copy(S2EncoderSnapshot *out)
{
    if ((out == NULL) || !atomic_load_explicit(&ready, memory_order_acquire))
    {
        return false;
    }
    for (unsigned attempt = 0u; attempt < 2u; ++attempt)
    {
        unsigned before = atomic_load_explicit(&version,
            memory_order_seq_cst);
        S2EncoderSnapshot copy = {
            .left_count = atomic_load_explicit(&left_value,
                memory_order_seq_cst),
            .right_count = atomic_load_explicit(&right_value,
                memory_order_seq_cst),
            .errors = atomic_load_explicit(&error_value,
                memory_order_seq_cst),
            .sequence = atomic_load_explicit(&sequence_value,
                memory_order_seq_cst),
            .captured_us = atomic_load_explicit(&time_value,
                memory_order_seq_cst),
            .valid = true
        };
        if (((before & 1u) == 0u) &&
            (before == atomic_load_explicit(&version, memory_order_seq_cst)))
        {
            copy.valid = copy.errors == 0u;
            *out = copy;
            return true;
        }
    }
    return false;
}
