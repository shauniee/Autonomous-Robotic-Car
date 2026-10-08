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
typedef struct { uint32_t serviced_ms, sequence; bool stopped; } MotionHealth;
ER s2_publish(MotionRequest request);
ER s2_stop(void); /* Persistent event flag plus independent wake-up. */
ER s2_reset_stop(void); /* Called only on a fresh local reset, while braked. */
ER s2_health(MotionHealth *health);
#endif
