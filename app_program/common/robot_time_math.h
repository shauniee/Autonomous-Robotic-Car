/*
 *	robot_time_math.h
 *	Wrap-safe interval arithmetic on uint32_t stamps. Kernel-free so it can
 *	be tested on the host; robot_time.h adds the two clock readers.
 */

#ifndef ROBOT_TIME_MATH_H
#define ROBOT_TIME_MATH_H

#include <stdbool.h>
#include <stdint.h>

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
static inline uint32_t robot_time_next_delay(uint32_t *release, uint32_t period, uint32_t now)
{
    if (robot_time_reached(now, *release)) {
        *release += period;
        if (robot_time_reached(now, *release)) {
            *release = now + period;
        }
    }
    return *release - now;
}

#endif /* ROBOT_TIME_MATH_H */
