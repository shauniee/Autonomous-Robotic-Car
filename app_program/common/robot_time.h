#ifndef ROBOT_TIME_H
#define ROBOT_TIME_H
#include "../app.h"
#include <stdbool.h>
#include <stdint.h>
static inline bool robot_now_ms(uint32_t *now)
{
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK) { return false; }
    *now = time.lo;
    return true;
}
static inline bool robot_due(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}
#endif
