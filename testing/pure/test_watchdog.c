#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../../app_program/common/robot_watchdog.h"

int main(void)
{
    const uint32_t max_age = ROBOT_WATCHDOG_PROGRESS_MAX_AGE_MS;
    /* Never arm/feed without both task heartbeats. */
    assert(!robot_watchdog_progress_fresh(1000u, 0u, 1000u, max_age));
    assert(!robot_watchdog_progress_fresh(1000u, 1000u, 0u, max_age));
    assert(robot_watchdog_progress_fresh(1000u, 950u, 990u, max_age));
    assert(!robot_watchdog_progress_fresh(1000u, 1000u - max_age - 1u, 990u, max_age));
    assert(!robot_watchdog_progress_fresh(1000u, 990u, 1000u - max_age - 1u, max_age));
    /* Progress remains fresh across a uint32 millisecond timer rollover. */
    assert(robot_watchdog_progress_fresh(15u, UINT32_MAX - 10u, 12u, max_age));
    /* A counter moved into the future is not mistaken for fresh progress. */
    assert(!robot_watchdog_progress_fresh(100u, 101u, 90u, max_age));
    puts("watchdog_progress: PASS (policy only; no hardware exercised)");
    return 0;
}
