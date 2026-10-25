/*
 *	drv_barcode.h
 *	GP16 edge capture. The dispatcher (core 1) calls the edge handler, which
 *	pushes {time, both encoder counts, level} into a single-producer /
 *	single-consumer ring and returns. The task drains the ring.
 */

#ifndef DRV_BARCODE_H
#define DRV_BARCODE_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

typedef struct
{
    uint32_t sampled_us;
    uint32_t left_count, right_count;
    bool high;	/* GP16 level after the edge */
} barcode_edge_t;

/* Core-1 task only: configure GP16 and claim it on the GPIO dispatcher. The
   edge IRQ stays disabled until drv_barcode_enable(true). */
bool drv_barcode_init(void);
/* Core-1 task only. Enabling clears the ring and the overflow/order counters
   baseline so each scan session starts empty. */
void drv_barcode_enable(bool enabled);
/* Task side: pop one edge. False when empty. */
bool drv_barcode_take(barcode_edge_t *edge);
/* ISR-written counters (single writer: the handler). Compare with a stored
   value; a change means the ring overflowed / both edges arrived together. */
uint32_t drv_barcode_overflow_count(void);
uint32_t drv_barcode_order_lost_count(void);

#endif
