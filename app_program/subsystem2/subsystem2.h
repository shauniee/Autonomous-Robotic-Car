/* Mahesha: exclusive motor writer and asynchronous stop/reset interface. */
#ifndef SUBSYSTEM2_H
#define SUBSYSTEM2_H

#include "../app.h"
#include "motion_policy.h"

#define S2_PRI_MOTION 13

typedef struct
{
    uint32_t serviced_ms;
    uint32_t sequence;
    uint32_t stop_generation;
    uint32_t brake_ack_generation;
    uint32_t reset_ack_generation;
    float applied_left;
    float applied_right;
    bool stopped;
    bool output_braked;
    bool test_inhibited;
    bool driver_ok;
    char reason[32];
} MotionHealth;

ER subsystem2_init(void);
ER subsystem2_start(void);
ER s2_publish(MotionRequest request);
/* Independent of the request mutex. Each call invalidates prior generations.
   Acceptance is not a brake acknowledgement; an in-flight write may finish. */
ER s2_stop(void);
/* Asynchronous: only Motion may clear the matching acknowledged stop.
   Wait for reset_ack_generation and !stopped before publishing a new target. */
ER s2_request_reset(uint32_t expected_generation);
/* Nonblocking task-context snapshot; destination is unchanged on failure.
   Applied duty/brake acknowledgement are software output, not wheel motion. */
bool s2_motion_copy(MotionHealth *health);
ER s2_health(MotionHealth *health);

#endif
