/* Kernel-free packed Motion progress/stop snapshot, guarded by S2 mutex. */
#ifndef MOTION_HEALTH_H
#define MOTION_HEALTH_H
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t serviced_ms, sequence;
    uint8_t flags; /* bit 0 stop latched, bit 1 brake acknowledged */
} MotionHealth;

enum { MOTION_HEALTH_STOPPED_BIT = 1u, MOTION_HEALTH_BRAKED_BIT = 2u };
static inline bool motion_health_stopped(const MotionHealth *h)
{ return (h->flags & MOTION_HEALTH_STOPPED_BIT) != 0u; }
static inline bool motion_health_brake_acknowledged(const MotionHealth *h)
{ return (h->flags & MOTION_HEALTH_BRAKED_BIT) != 0u; }
_Static_assert(sizeof(MotionHealth) == 12u, "unexpected MotionHealth layout");
#endif
