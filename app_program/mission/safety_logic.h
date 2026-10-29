/*
 *	safety_logic.h
 *	Kernel-free Safety decisions: which causes demand a stop, and when a
 *	latched stop may be reset. Network faults are report-only and never
 *	appear here.
 */

#ifndef SAFETY_LOGIC_H
#define SAFETY_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/buttons.h"
#include "../common/robot_time_math.h"
#include "../common/robot_data.h"

#define SAFETY_CAUSE_STOP_BUTTON	(1u << 0)	/* GP21 held */
#define SAFETY_CAUSE_FAULT		(1u << 1)	/* a non-Network owner fault bit */
#define SAFETY_CAUSE_STALE_LINE		(1u << 2)	/* IR snapshot older than the limit */
#define SAFETY_CAUSE_STALE_MOTION	(1u << 3)	/* Motion has not serviced recently */

typedef struct
{
    bool line_required;
    bool stop_button;		/* GP21 down */
    bool fault_pending;		/* rd_fault_stop_pending() */
    bool line_seen;		/* the IR producer has published at least once */
    uint32_t line_age_ms;	/* age of the newest IR acquisition */
    bool motion_seen;		/* Motion has serviced at least once */
    uint32_t motion_age_ms;	/* age of Motion's last serviced release */
    uint32_t uptime_ms;
} SafetyInputs;

/* Causes present right now. A producer that has never published is excused
   only during the boot grace period. */
static inline uint32_t safety_causes(const SafetyInputs *in)
{
    uint32_t causes = 0u;
    bool grace = in->uptime_ms < SAFETY_BOOT_GRACE_MS;
    if (in->stop_button) causes |= SAFETY_CAUSE_STOP_BUTTON;
    if (APP_NO_GUARDS) return causes;	/* bench build: GP21 is the only stop */
    if (in->fault_pending) causes |= SAFETY_CAUSE_FAULT;
    if (in->line_required && (in->line_seen ? in->line_age_ms > SAFETY_STALE_MS : !grace)) causes |= SAFETY_CAUSE_STALE_LINE;
    if (in->motion_seen ? in->motion_age_ms > SAFETY_STALE_MS : !grace) causes |= SAFETY_CAUSE_STALE_MOTION;
    return causes;
}

/* A reset on an eligible GP21 release needs every cause cleared
   and the brake serviced. It only releases the latch; it never starts motion. */
static inline bool safety_reset_allowed(uint32_t current_causes, bool brake_acknowledged)
{
    return current_causes == 0u && brake_acknowledged;
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

static inline bool safety_reset_step(uint8_t *state, ButtonEvent event,
    uint32_t causes, bool brake_acknowledged)
{
    if (*state == SAFETY_RESET_BOOT && causes != 0u)
    {
        *state = SAFETY_RESET_PRESS;
    }
    if ((causes & ~SAFETY_CAUSE_STOP_BUTTON) != 0u)
    {
        *state = SAFETY_RESET_PRESS;
    }
    else if (event == BUTTON_EVENT_PRESSED)
    {
        *state = SAFETY_RESET_RELEASE;
    }
    bool request = *state == SAFETY_RESET_BOOT ||
        (*state == SAFETY_RESET_RELEASE && event == BUTTON_EVENT_RELEASED);
    if (event == BUTTON_EVENT_RELEASED)
    {
        *state = SAFETY_RESET_PRESS;
    }
    return request && safety_reset_allowed(causes, brake_acknowledged);
}

#endif /* SAFETY_LOGIC_H */
