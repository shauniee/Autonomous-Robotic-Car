/*
 *    safety_logic.h
 *    Kernel-free Safety decisions: which causes demand a stop, and when a
 *    latched stop may be reset. Network faults are report-only and never
 *    appear here.
 */

#ifndef SAFETY_LOGIC_H
#define SAFETY_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/buttons.h"
#include "../common/robot_time_math.h"
#include "../common/robot_data.h"

#define SAFETY_CAUSE_STOP_BUTTON    (1u << 0)    /* GP21 held */
/* a non-Network owner fault bit */
#define SAFETY_CAUSE_FAULT        (1u << 1)
/* IR snapshot older than the limit */
#define SAFETY_CAUSE_STALE_LINE        (1u << 2)
/* Motion has not serviced recently */
#define SAFETY_CAUSE_STALE_MOTION    (1u << 3)

typedef struct
{
    uint32_t line_age_ms;    /* age of the newest IR acquisition */
    uint32_t motion_age_ms;    /* age of Motion's last serviced release */
    uint32_t uptime_ms;
    bool line_required : 1;
    bool stop_button : 1; /* GP21 down */
    bool fault_pending : 1; /* rd_fault_stop_pending() */
    bool line_seen : 1; /* the IR producer has published at least once */
    bool motion_seen : 1; /* Motion has serviced at least once */
} SafetyInputs_t;

/* Causes present right now. A producer that has never published is excused
   only during the boot grace period. */
static inline uint32_t safety_causes(const SafetyInputs_t *in)
{
    uint32_t causes = 0u;
    bool grace = in->uptime_ms < SAFETY_BOOT_GRACE_MS;
    if (in->stop_button)
    {
        causes |= SAFETY_CAUSE_STOP_BUTTON;
    }
    if (APP_NO_GUARDS)
    {
        return causes; /* bench build: GP21 is the only stop */
    }
    if (in->fault_pending)
    {
        causes |= SAFETY_CAUSE_FAULT;
    }
    if (in->line_required &&
        (in->line_seen ? in->line_age_ms > SAFETY_STALE_MS : !grace))
    {
        causes |= SAFETY_CAUSE_STALE_LINE;
    }
    if (in->motion_seen ? in->motion_age_ms > SAFETY_STALE_MS : !grace)
    {
        causes |= SAFETY_CAUSE_STALE_MOTION;
    }
    return causes;
}

/* A reset on an eligible GP21 release needs every cause cleared
   and the brake serviced. It only releases the latch; it never starts motion.
   */
static inline bool safety_reset_allowed(uint32_t current_causes,
    bool brake_acknowledged)
{
    return 0u == current_causes && brake_acknowledged;
}

/* Three exclusive handshake states, stored in one byte by Mission.
   Boot may release inhibition once. Every subsequent reset requires a new
   GP21 press with causes clear, followed by release and serviced braking. */
typedef enum
{
    SAFETY_RESET_BOOT,
    SAFETY_RESET_PRESS,
    SAFETY_RESET_RELEASE
} safety_reset_state_t;

static inline bool safety_reset_step(uint8_t *state, ButtonEvent_t event,
    uint32_t causes, bool brake_acknowledged)
{
    if (SAFETY_RESET_BOOT == *state && causes != 0u)
    {
        *state = SAFETY_RESET_PRESS;
    }
    if ((causes & ~SAFETY_CAUSE_STOP_BUTTON) != 0u)
    {
        *state = SAFETY_RESET_PRESS;
    }
    else if (BUTTON_EVENT_PRESSED == event)
    {
        *state = SAFETY_RESET_RELEASE;
    }
    bool request = SAFETY_RESET_BOOT == *state ||
        (SAFETY_RESET_RELEASE == *state && BUTTON_EVENT_RELEASED == event);
    if (BUTTON_EVENT_RELEASED == event)
    {
        *state = SAFETY_RESET_PRESS;
    }
    return request && safety_reset_allowed(causes, brake_acknowledged);
}

#endif /* SAFETY_LOGIC_H */
