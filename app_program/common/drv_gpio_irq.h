#ifndef DRV_GPIO_IRQ_H
#define DRV_GPIO_IRQ_H
#include "../app.h"
#include <stdbool.h>
#include <stdint.h>
typedef void (*gpio_edge_handler_t)(unsigned pin, uint32_t edges,
                                    uint32_t sampled_us);
/* Core-1 task callers. Capture on core 1 only; never use SDK IRQ shims. */
ER drv_gpio_irq_init(void);
bool drv_gpio_irq_claim(unsigned pin, gpio_edge_handler_t handler);
bool drv_gpio_irq_enable(unsigned pin, bool enabled);
#endif
