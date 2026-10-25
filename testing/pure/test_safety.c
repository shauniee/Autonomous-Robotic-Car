/* Host test for app_program/subsystem3/safety_logic.h (issue #37). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/subsystem3/safety_logic.h"

static SafetyInputs healthy(void)
{
    return (SafetyInputs){.line_seen = true, .line_age_ms = 20u, .motion_seen = true, .motion_age_ms = 20u,
                          .uptime_ms = 10000u};
}

int main(void)
{
    SafetyInputs in = healthy();
    assert(safety_causes(&in) == 0u);

    in.stop_button = true;
    assert(safety_causes(&in) == SAFETY_CAUSE_STOP_BUTTON);

    /* An injected fault in each owner byte latches stop; Network never does. */
    for (unsigned owner = 0u; owner < RD_OWNER_COUNT; ++owner) {
        rd_fault_set((rd_owner_t)owner, 0x01u);
        in = healthy();
        in.fault_pending = rd_fault_stop_pending();
        assert((safety_causes(&in) == SAFETY_CAUSE_FAULT) == (owner != RD_OWNER_NETWORK));
        assert((safety_causes(&in) == 0u) == (owner == RD_OWNER_NETWORK));
        rd_fault_clear((rd_owner_t)owner, 0x01u);
    }

    /* Freshness: at the limit is fresh, one past is stale, per producer. */
    in = healthy(); in.line_age_ms = SAFETY_STALE_MS;
    assert(safety_causes(&in) == 0u);
    in.line_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == SAFETY_CAUSE_STALE_LINE);
    in = healthy(); in.motion_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == SAFETY_CAUSE_STALE_MOTION);
    in = healthy(); in.line_age_ms = in.motion_age_ms = SAFETY_STALE_MS + 1u;
    assert(safety_causes(&in) == (SAFETY_CAUSE_STALE_LINE | SAFETY_CAUSE_STALE_MOTION));	/* independent */

    /* A producer that never published is excused only during boot grace. */
    in = healthy(); in.line_seen = false; in.motion_seen = false; in.uptime_ms = SAFETY_BOOT_GRACE_MS - 1u;
    assert(safety_causes(&in) == 0u);
    in.uptime_ms = SAFETY_BOOT_GRACE_MS;
    assert(safety_causes(&in) == (SAFETY_CAUSE_STALE_LINE | SAFETY_CAUSE_STALE_MOTION));

    /* Reset needs a fresh press, no current cause and a serviced brake. */
    assert(safety_reset_allowed(0u, true, true));
    assert(!safety_reset_allowed(0u, true, false));
    assert(!safety_reset_allowed(0u, false, true));
    assert(!safety_reset_allowed(SAFETY_CAUSE_FAULT, true, true));
    assert(!safety_reset_allowed(SAFETY_CAUSE_STOP_BUTTON, true, true));

    puts("safety: PASS");
    return 0;
}
