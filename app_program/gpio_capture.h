#ifndef GPIO_CAPTURE_H
#define GPIO_CAPTURE_H

#include "app.h"
#include <stdbool.h>
#include <stdint.h>

/* Init/register on startup core; enable only on application core. Callbacks
   run in IRQ context and must never block or call task-context APIs. */
typedef void (*GpioCaptureCallback)(uint32_t levels, uint32_t changed,
    uint32_t ambiguous, uint32_t timestamp_us);
ER gpio_capture_init(void);
ER gpio_capture_register(uint32_t mask, GpioCaptureCallback callback);
ER gpio_capture_enable(void);
uint32_t gpio_capture_time_us(void);

#endif
