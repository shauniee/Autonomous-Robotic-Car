/*
 *	robot_time.h
 *	The only time helpers for application tasks.  Two clocks, never mixed:
 *	  robot_time_ms()  kernel milliseconds: freshness, control periods, waits
 *	  robot_time_us()  RP2040 TIMER microseconds: barcode and Echo edges
 *	Stored stamps are uint32_t.  Compare them only through robot_time_elapsed()
 *	or robot_time_reached(), which stay correct across wrap.
 */

#ifndef ROBOT_TIME_H
#define ROBOT_TIME_H

#include <stdbool.h>
#include <stdint.h>
#include "../app.h"
#include "robot_time_math.h"

/* Kernel milliseconds since boot. tk_get_otm() always returns E_OK
   (kernel/tkernel/time_calls.c), so no error path is reported here. */
static inline uint32_t robot_time_ms(void)
{
    SYSTIM now;
    (void)tk_get_otm(&now);
    return now.lo;
}

/* Low 32 bits of the free-running 1 MHz hardware TIMER. */
static inline uint32_t robot_time_us(void)
{
    return in_w(TIMER_TIMERAWL);
}

#endif /* ROBOT_TIME_H */
