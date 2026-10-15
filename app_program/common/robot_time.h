#ifndef ROBOT_TIME_H
#define ROBOT_TIME_H
#include "../app.h"
#include <stdbool.h>
#include <stdint.h>
static inline bool robot_now_ms(uint32_t *now)
{
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK)
    {
        return false;
    }
    *now = time.lo;
    return true;
}
static inline bool robot_due(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}
/* Core 1 has no SysTick counter in this SMP port. Use the shared 1 MHz
   TIMER for short GPIO pulses; a stopped TIMER must not create a busy loop.
   This is a pulse helper, not a task delay. Interrupts remain enabled. */
static inline bool robot_pulse_wait_us(uint32_t duration_us)
{
    if (duration_us > 100u)
    {
        return false;
    }
    uint32_t started = in_w(TIMER_TIMERAWL);
    for (unsigned budget = 0u; budget < 20000u; ++budget)
    {
        if (in_w(TIMER_TIMERAWL) - started >= duration_us)
        {
            return true;
        }
    }
    return false;
}
#endif
