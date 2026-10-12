/* Mahesha: exclusive motor writer and request/stop interface. */
#ifndef SUBSYSTEM2_H
#define SUBSYSTEM2_H
#ifdef BENCH_KERNEL_HOST_TEST
#include "kernel_io.h"
#else
#include "../app.h"
#endif
#include "motion_policy.h"
#define S2_PRI_MOTION 13
ER subsystem2_init(void);
ER subsystem2_start(void);
/* Progress records successful output processing, not measured motion. */
typedef struct
{
    uint32_t serviced_ms, sequence;
    bool stopped;
    bool brake_acknowledged; /* Successful brake service of latest stop. */
} MotionHealth;
ER s2_publish(MotionRequest request);
/* Core-1 callers only: persistent flag plus independent wake-up.
   Motion serializes its final flag check/output against same-core stops. */
ER s2_stop(void);
/* Fresh core-1 local action only. E_OBJ until latest stop is serviced.
   Success clears cached work; it does not command movement. */
ER s2_reset_stop(void);
ER s2_health(MotionHealth *health);
#endif
