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

/* Interval since `then`; valid while the interval is below 2^31 units. */
static inline uint32_t robot_time_elapsed(uint32_t now, uint32_t then)
{
    return now - then;
}

/* True when `now` is at or after `deadline`, across wrap. */
static inline bool robot_time_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

/* Fixed-rate release.  Call once per cycle with the current kernel ms and
   sleep for the returned delay.  *release is the grid anchor for the next
   cycle, advanced by one period each time it is reached.  An overrun
   re-anchors to now, so missed periods are skipped rather than run back to
   back. */
static inline RELTIM robot_time_next_delay(uint32_t *release, uint32_t period, uint32_t now)
{
    if (robot_time_reached(now, *release)) {
        *release += period;
        if (robot_time_reached(now, *release)) {
            *release = now + period;
        }
    }
    return (RELTIM)(*release - now);
}

#endif /* ROBOT_TIME_H */
