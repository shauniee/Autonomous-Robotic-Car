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
#include "robot_config.h"

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

/* Every LOOP_TIMING_REPORT_MS print one console line for S3-M02, then start
   a new window. Cheap enough for every build; silent without a console. */
static inline void loop_timing_report(LoopTiming *t, const char *name, uint32_t period_us,
    uint32_t *report_due, uint32_t now_ms)
{
    if (!robot_time_reached(now_ms, *report_due))
    {
        return;
    }
    tm_printf((UB *)"[TIMING] task=%s max_us=%u period_us=%u overruns=%u loops=%u\n",
        name, (unsigned)t->max_us, (unsigned)period_us, (unsigned)t->overruns, (unsigned)t->count);
    *t = (LoopTiming){0};
    *report_due = now_ms + LOOP_TIMING_REPORT_MS;
}

#endif /* ROBOT_TIME_H */
