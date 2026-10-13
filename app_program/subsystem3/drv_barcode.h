#ifndef DRV_BARCODE_H
#define DRV_BARCODE_H

#include "barcode_types.h"

#define BARCODE_EDGE_FALL 4u
#define BARCODE_EDGE_RISE 8u
#define BARCODE_GPIO 16u

typedef struct
{
    barcode_encoder_t encoder;
    uint32_t sampled_us;
    uint32_t mapping_revision;
    bool level;
} barcode_edge_t;

/* Reset/enable: core-1 task with local interrupts masked. No GPIO-bank or
   NVIC registration here: the agreed dispatcher owns those shared resources. */
void drv_barcode_reset(void);
bool drv_barcode_gpio_init(void);
void drv_barcode_enable(bool enabled);
bool drv_barcode_take(barcode_edge_t *edge);
bool drv_barcode_empty(void);
uint32_t drv_barcode_faults(void);
barcode_result_t drv_barcode_fault_reason(void);
void drv_barcode_mapping_revision(uint32_t revision);

/* Integration entry: core-1 GPIO dispatcher supplies GP16 edge flags and
   coherent encoder context. Dispatcher acknowledges ONLY its own GPIO bits.
   Both edge flags together mean ordering was lost, never two guessed edges.
   Must be called once per actual edge, never for periodic level polling. */
void drv_barcode_edge_isr(uint32_t events, uint32_t sampled_us,
    const barcode_encoder_t *encoder);

#endif
