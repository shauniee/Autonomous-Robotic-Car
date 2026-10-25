#include "drv_encoder.h"
#include <stdatomic.h>
#include "../app.h"
#include "../common/drv_gpio_irq.h"

/* Channel pins per wheel: [0] = A (high bit of the level), [1] = B. */
static const unsigned pins[ENCODER_WHEELS][2] = {
    {ENC_LEFT_A_PIN, ENC_LEFT_B_PIN},
    {ENC_RIGHT_A_PIN, ENC_RIGHT_B_PIN},
};
static const int sign[ENCODER_WHEELS] = {LEFT_ENCODER_SIGN, RIGHT_ENCODER_SIGN};

/* Each storage unit has one writer: the dispatcher handler on core 1. */
static _Atomic uint32_t counts[ENCODER_WHEELS];
static _Atomic uint32_t invalid_edges;
static unsigned previous[ENCODER_WHEELS];	/* ISR-private */

static unsigned level(unsigned wheel)
{
    return (unsigned)gpio_get_val(pins[wheel][0]) * 2u + (unsigned)gpio_get_val(pins[wheel][1]);
}

static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)sampled_us;
    unsigned wheel = (pin == ENC_LEFT_A_PIN || pin == ENC_LEFT_B_PIN) ? ENCODER_LEFT : ENCODER_RIGHT;
    unsigned current = level(wheel);
    int delta = enc_quad_step(previous[wheel], current);
    previous[wheel] = current;
    if (delta == ENC_QUAD_INVALID || edges == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)) {
        uint32_t n = atomic_load_explicit(&invalid_edges, memory_order_relaxed);
        atomic_store_explicit(&invalid_edges, n + 1u, memory_order_release);
    } else if (delta != 0) {
        uint32_t c = atomic_load_explicit(&counts[wheel], memory_order_relaxed);
        atomic_store_explicit(&counts[wheel], enc_advance(c, delta * sign[wheel]), memory_order_release);
    }
}

bool drv_encoder_init(void)
{
    if (drv_gpio_irq_init() < E_OK) {
        return false;
    }
    for (unsigned wheel = 0u; wheel < ENCODER_WHEELS; ++wheel) {
        for (unsigned channel = 0u; channel < 2u; ++channel) {
            unsigned pin = pins[wheel][channel];
            if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK || !drv_gpio_irq_claim(pin, edge)) {
                return false;
            }
        }
        previous[wheel] = level(wheel);
    }
    for (unsigned wheel = 0u; wheel < ENCODER_WHEELS; ++wheel) {
        for (unsigned channel = 0u; channel < 2u; ++channel) {
            if (!drv_gpio_irq_enable(pins[wheel][channel], true)) {
                return false;
            }
        }
    }
    return true;
}

uint32_t drv_encoder_count(encoder_wheel_t wheel)
{
    if ((unsigned)wheel >= ENCODER_WHEELS) {
        return 0u;
    }
    return atomic_load_explicit(&counts[wheel], memory_order_acquire);
}

uint32_t drv_encoder_invalid_edges(void)
{
    return atomic_load_explicit(&invalid_edges, memory_order_acquire);
}
