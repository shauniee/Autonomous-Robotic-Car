/* Kernel-free stop/reset decision shared with the real Motion service. */
#ifndef MOTION_STOP_H
#define MOTION_STOP_H

#include <stdbool.h>
#include <stdint.h>

static inline bool motion_stop_reset_allowed(bool inhibited, bool stopped,
    uint32_t requested, uint32_t serviced, bool cause_pending)
{
    return !inhibited && stopped && requested == serviced && !cause_pending;
}

#endif /* MOTION_STOP_H */
