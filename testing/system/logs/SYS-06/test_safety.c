/* Host test for app_program/mission/safety_logic.h (issue #37). */
#include <assert.h>
#include <stdio.h>
#include "../../../../app_program/mission/safety_logic.h"
#include "../../../../app_program/motion/motion.h"

static SafetyInputs_t healthy(void)
{
    return (SafetyInputs_t){.line_required = true, .line_seen = true, .line_age_ms = 20u, .motion_seen = true, .motion_age_ms = 20u,
                          .uptime_ms = 10000u};
}

int main(void)
{
    SafetyInputs_t in = healthy();
    assert(safety_causes(&in) == 0u);

    in.stop_button = true;
    assert(safety_causes(&in) == SAFETY_CAUSE_STOP_BUTTON);

    /* Only run-stopping bits latch a stop; report-only bits (no echo,
       network) never do. The full policy table is in test_robot_data.c. */
    rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_LIFTED);
    in = healthy(); in.fault_pending = rd_fault_stop_pending();
    assert(safety_causes(&in) == SAFETY_CAUSE_FAULT);
    rd_fault_clear(RD_OWNER_TERRAIN, RD_FAULT_LIFTED);
    rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
    rd_fault_set(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);
    in = healthy(); in.fault_pending = rd_fault_stop_pending();
    assert(safety_causes(&in) == 0u);
    rd_fault_clear(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
    rd_fault_clear(RD_OWNER_NETWORK, RD_FAULT_WIFI_LOST);

    /* Freshness: at the limit is fresh, one past is stale, per producer. */
    in = healthy(); in.line_age_ms = SAFETY_STALE_MS;
    assert(safety_causes(&in) == 0u);
    in.line_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == SAFETY_CAUSE_STALE_LINE);
    in = healthy(); in.motion_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == SAFETY_CAUSE_STALE_MOTION);
    in = healthy(); in.line_age_ms = in.motion_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == (SAFETY_CAUSE_STALE_LINE | SAFETY_CAUSE_STALE_MOTION));	/* independent */

    in = healthy(); in.line_required = false;
    in.line_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == 0u); /* encoder turns need no visible line */
    in.motion_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == SAFETY_CAUSE_STALE_MOTION);

    /* A producer that never published is excused only during boot grace. */
    in = healthy(); in.line_seen = false; in.motion_seen = false; in.uptime_ms = SAFETY_BOOT_GRACE_MS - 1u;
    assert(safety_causes(&in) == 0u);
    in.uptime_ms = SAFETY_BOOT_GRACE_MS;
    assert(safety_causes(&in) == (SAFETY_CAUSE_STALE_LINE | SAFETY_CAUSE_STALE_MOTION));

    /* Reset needs a fresh press, no current cause and a serviced brake. */
    assert(safety_reset_allowed(0u, true));
    assert(!safety_reset_allowed(0u, false));
    assert(!safety_reset_allowed(SAFETY_CAUSE_FAULT, true));
    assert(!safety_reset_allowed(SAFETY_CAUSE_STOP_BUTTON, true));

    uint8_t reset = SAFETY_RESET_BOOT;
    assert(!safety_reset_step(&reset, BUTTON_EVENT_NONE, 0u, false));
    assert(safety_reset_step(&reset, BUTTON_EVENT_NONE, 0u, true));
    reset = SAFETY_RESET_PRESS;
    assert(!safety_reset_step(&reset, BUTTON_EVENT_NONE, 0u, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_IGNORED_PRESS,
        SAFETY_CAUSE_STOP_BUTTON, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_RELEASED, 0u, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_PRESSED,
        SAFETY_CAUSE_STOP_BUTTON, true));
    assert(safety_reset_step(&reset, BUTTON_EVENT_RELEASED, 0u, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_RELEASED, 0u, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_PRESSED,
        SAFETY_CAUSE_STOP_BUTTON | SAFETY_CAUSE_FAULT, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_RELEASED, 0u, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_PRESSED,
        SAFETY_CAUSE_STOP_BUTTON, true));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_RELEASED, 0u, false));
    assert(!safety_reset_step(&reset, BUTTON_EVENT_NONE, 0u, true));

    /* 2026-10-29: the real reset service uses this predicate while interrupts
       are masked. A stop arriving after the acknowledged brake refuses reset. */
    assert(!motion_stop_reset_allowed(false, true, 1u, 0u, false)); /* boot */
    assert(!motion_stop_reset_allowed(false, true, 2u, 1u, false));
    assert(motion_stop_reset_allowed(false, true, 2u, 2u, false));
    assert(!motion_stop_reset_allowed(true, true, 2u, 2u, false));
    assert(!motion_stop_reset_allowed(false, false, 2u, 2u, false));
    assert(!motion_stop_reset_allowed(false, true, 2u, 2u, true));
    assert(!motion_stop_reset_allowed(false, true, 0u, UINT32_MAX, false));
    assert(motion_stop_reset_allowed(false, true, 0u, 0u, false));
    puts("reset brake ordering: PASS");
    return 0;
}
