#include "drv_barcode.h"
#include "../app.h"
#include <stdatomic.h>

static barcode_edge_t edges[BARCODE_EDGE_CAPACITY];
static atomic_uint head;
static atomic_uint tail;
static atomic_uint faults;
static atomic_uint fault_reason;
static atomic_uint mapping_revision;
static volatile bool enabled; /* Same core, changed with local IRQs masked. */

bool drv_barcode_gpio_init(void)
{
    if (gpio_set_pin(BARCODE_GPIO, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }
    /* Mounted comparator must supply a verified safe pull-up/output level.
       No GPIO-bank reset or other owner's pin/register configuration. */
    clr_w(GPIO(BARCODE_GPIO), GPIO_PUE | GPIO_PDE);
    return true;
}

void drv_barcode_reset(void)
{
    enabled = false;
    atomic_store_explicit(&head, 0u, memory_order_release);
    atomic_store_explicit(&tail, 0u, memory_order_release);
    atomic_store_explicit(&faults, 0u, memory_order_release);
    atomic_store_explicit(&fault_reason, BARCODE_NO_RESULT,
        memory_order_release);
}

void drv_barcode_enable(bool value)
{
    enabled = value;
}

uint32_t drv_barcode_faults(void)
{
    return atomic_load_explicit(&faults, memory_order_acquire);
}

barcode_result_t drv_barcode_fault_reason(void)
{
    return (barcode_result_t)atomic_load_explicit(&fault_reason,
        memory_order_acquire);
}

void drv_barcode_mapping_revision(uint32_t revision)
{
    atomic_store_explicit(&mapping_revision, revision, memory_order_release);
}

bool drv_barcode_empty(void)
{
    return atomic_load_explicit(&head, memory_order_acquire) ==
        atomic_load_explicit(&tail, memory_order_acquire);
}

void drv_barcode_edge_isr(uint32_t events, uint32_t sampled_us,
    const barcode_encoder_t *encoder)
{
    if (!enabled)
    {
        return;
    }
    unsigned write = atomic_load_explicit(&head, memory_order_relaxed);
    unsigned read = atomic_load_explicit(&tail, memory_order_acquire);
    if (((events != BARCODE_EDGE_RISE) && (events != BARCODE_EDGE_FALL)) ||
        (encoder == NULL) || ((unsigned)(write - read) >=
            BARCODE_EDGE_CAPACITY))
    {
        /* One ISR writer; only reset clears the fault with IRQs masked.
           Aligned atomic loads/stores only: no retry loop or SMP lock. */
        barcode_result_t reason = encoder == NULL ? BARCODE_BAD_ENCODER :
            ((unsigned)(write - read) >= BARCODE_EDGE_CAPACITY ?
                BARCODE_CAPTURE_OVERFLOW : BARCODE_BAD_EDGE);
        atomic_store_explicit(&fault_reason, reason, memory_order_release);
        atomic_store_explicit(&faults, BARCODE_FAULT_CAPTURE,
            memory_order_release);
        enabled = false;
        return;
    }
    edges[write % BARCODE_EDGE_CAPACITY] = (barcode_edge_t)
    {
        .encoder = *encoder,
        .sampled_us = sampled_us,
        .mapping_revision = atomic_load_explicit(&mapping_revision,
            memory_order_acquire),
        .level = events == BARCODE_EDGE_RISE
    };
    atomic_store_explicit(&head, write + 1u, memory_order_release);
}

bool drv_barcode_take(barcode_edge_t *edge)
{
    if (edge == NULL)
    {
        return false;
    }
    unsigned read = atomic_load_explicit(&tail, memory_order_relaxed);
    unsigned write = atomic_load_explicit(&head, memory_order_acquire);
    if (read == write)
    {
        return false;
    }
    *edge = edges[read % BARCODE_EDGE_CAPACITY];
    atomic_store_explicit(&tail, read + 1u, memory_order_release);
    return true;
}
