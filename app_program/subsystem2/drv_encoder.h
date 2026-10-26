/*
 *	drv_encoder.h
 *	Quadrature decoder for both wheels, driven by the shared GPIO IRQ
 *	dispatcher on core 1. The ISR is the only writer of the counts. Counts
 *	wrap at ENC_COUNT_BITS; see encoder_math.h for travel arithmetic.
 */

#ifndef DRV_ENCODER_H
#define DRV_ENCODER_H

#include <stdbool.h>
#include <stdint.h>
#include "encoder_math.h"

typedef enum { ENCODER_LEFT, ENCODER_RIGHT, ENCODER_WHEELS } encoder_wheel_t;

/* Core-1 task only (the dispatcher needs core 1). False if any pin could not
   be claimed. Uses GP0/GP1: build with CONSOLE=usb_cdc CRASH_REPORT=0. */
bool drv_encoder_init(void);

/* Current wrapped count: one aligned load, callable from any task or core. */
uint32_t drv_encoder_count(encoder_wheel_t wheel);

/* Number of invalid quadrature transitions since boot (ISR is the only
   writer). Motion maps a change in this value to its encoder fault bit. */
uint32_t drv_encoder_invalid_edges(void);

#endif /* DRV_ENCODER_H */
