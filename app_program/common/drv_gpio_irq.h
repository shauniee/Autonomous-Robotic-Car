/*
 *  drv_gpio_irq.h
 *  Single GPIO bank IRQ entry shared by barcode, encoder and Echo edges.
 *  The dispatcher reads one timestamp, acknowledges the edge bits of every
 *  claimed pin and calls that pin's handler. Handlers capture and return:
 *  no kernel wait, no mutex, no tm_printf.
 */

#ifndef DRV_GPIO_IRQ_H
#define DRV_GPIO_IRQ_H

#include <stdbool.h>
#include <stdint.h>
#include "../app.h"

#define GPIO_IRQ_EDGE_FALL 4u   /* INTR/STATUS edge-low bit of a pin */
#define GPIO_IRQ_EDGE_RISE 8u   /* INTR/STATUS edge-high bit of a pin */
#define GPIO_IRQ_PIN_COUNT 30u

/* edges: GPIO_IRQ_EDGE_FALL and/or GPIO_IRQ_EDGE_RISE (both: order lost).
   sampled_us: robot_time_us() read once at the start of the dispatch. */
typedef void (*gpio_edge_handler_t)(unsigned pin, uint32_t edges,
                                    uint32_t sampled_us);

/* Core-1 task only (returns E_CTX elsewhere). Idempotent. */
ER drv_gpio_irq_init(void);

/* Register the handler for one pin. False if not initialised, the pin is out
   of range or already claimed by a different handler. */
bool drv_gpio_irq_claim(unsigned pin, gpio_edge_handler_t handler);

/* Enable or disable both edges of a claimed pin. Enabling clears stale
   edges first. */
bool drv_gpio_irq_enable(unsigned pin, bool enabled);

#endif /* DRV_GPIO_IRQ_H */
