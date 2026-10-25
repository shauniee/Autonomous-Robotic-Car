#include "drv_barcode.h"
#include <stdatomic.h>
#include "../app.h"
#include "../common/drv_gpio_irq.h"
#include "../subsystem2/drv_encoder.h"

static barcode_edge_t ring[BARCODE_EDGE_CAPACITY];
static _Atomic unsigned head;		/* written by the handler */
static _Atomic unsigned tail;		/* written by the task */
static _Atomic uint32_t overflows;	/* written by the handler */
static _Atomic uint32_t order_lost;	/* written by the handler */
static bool claimed;

static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)pin;
    if (edges == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))
    {
        /* Both flags together: ordering was lost, never guess two edges. */
        atomic_store_explicit(&order_lost, atomic_load_explicit(&order_lost, memory_order_relaxed) + 1u,
            memory_order_release);
        return;
    }
    unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&tail, memory_order_acquire);
    if ((unsigned)(write - read) >= BARCODE_EDGE_CAPACITY)
    {
        atomic_store_explicit(&overflows, atomic_load_explicit(&overflows, memory_order_relaxed) + 1u,
            memory_order_release);
        return;
    }
    ring[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t){
        .sampled_us = sampled_us,
        .left_count = drv_encoder_count(ENCODER_LEFT),
        .right_count = drv_encoder_count(ENCODER_RIGHT),
        .high = edges == GPIO_IRQ_EDGE_RISE};
    atomic_store_explicit(&head, write + 1u, memory_order_release);
}

bool drv_barcode_init(void)
{
    if (drv_gpio_irq_init() < E_OK || gpio_set_pin(BARCODE_GPIO, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }
    /* The mounted comparator must supply a verified safe level: no pulls. */
    clr_w(GPIO(BARCODE_GPIO), GPIO_PUE | GPIO_PDE);
    claimed = drv_gpio_irq_claim(BARCODE_GPIO, edge);
    return claimed;
}

void drv_barcode_enable(bool enabled)
{
    if (!claimed)
    {
        return;
    }
    (void)drv_gpio_irq_enable(BARCODE_GPIO, false);
    if (enabled)
    {
        /* Disabled: the handler cannot run, so the task may move both indices. */
        atomic_store_explicit(&tail, atomic_load_explicit(&head, memory_order_acquire), memory_order_release);
        (void)drv_gpio_irq_enable(BARCODE_GPIO, true);
    }
}

bool drv_barcode_take(barcode_edge_t *out)
{
    unsigned read = atomic_load_explicit(&tail, memory_order_relaxed);
    if (read == atomic_load_explicit(&head, memory_order_acquire))
    {
        return false;
    }
    *out = ring[read % BARCODE_EDGE_CAPACITY];
    atomic_store_explicit(&tail, read + 1u, memory_order_release);
    return true;
}

uint32_t drv_barcode_overflow_count(void)
{
    return atomic_load_explicit(&overflows, memory_order_acquire);
}

uint32_t drv_barcode_order_lost_count(void)
{
    return atomic_load_explicit(&order_lost, memory_order_acquire);
}
