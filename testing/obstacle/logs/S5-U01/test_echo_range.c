/* Host test for app_program/obstacle/obstacle_profile.h (issue #48). */
#include <assert.h>
#include <stdio.h>
#include "../../../../app_program/obstacle/obstacle.h"

int main(void)
{
    obstacle_range_sample_t sample = {.word = echo_pack(150u),
        .sampled_ms = 100u, .sequence = 1u};
    assert(obstacle_range_sample_fresh(&sample, 160u, 60u));
    assert(!obstacle_range_sample_fresh(&sample, 161u, 60u));
    assert(!obstacle_range_sample_fresh(&sample, 100u + 65536u, 60u));
    sample.sampled_ms = UINT32_MAX - 10u;
    assert(obstacle_range_sample_fresh(&sample, 5u, 60u));
    sample.sequence = 0u;
    assert(!obstacle_range_sample_fresh(&sample, 5u, 60u));
    assert(echo_range_mm(SONAR_MIN_PULSE_US - 1u) == ECHO_RANGE_INVALID);	/* too short */
    assert(echo_range_mm(SONAR_TIMEOUT_US) == ECHO_RANGE_INVALID);		/* at the timeout */
    assert(echo_range_mm(0u) == ECHO_RANGE_INVALID);
    uint32_t near_mm = echo_range_mm(SONAR_MIN_PULSE_US);
    assert(near_mm < ECHO_RANGE_INVALID && near_mm > 0u);
    assert(echo_range_mm(5830u) >= 995u && echo_range_mm(5830u) <= 1005u);	/* about 1 m */
    assert(echo_range_mm(SONAR_TIMEOUT_US - 1u) < ECHO_RANGE_INVALID);

    uint32_t w = echo_pack(1000u);
    assert(echo_valid(w) && echo_range(w) == 1000u && (w >> 14) == 0u);	/* spare bits zero */
    w = echo_pack(ECHO_RANGE_INVALID);					/* no echo is not clear space */
    assert(!echo_valid(w) && echo_range(w) == ECHO_RANGE_INVALID);
    w = echo_pack(0u);							/* minimum */
    assert(echo_valid(w) && echo_range(w) == 0u);
    w = echo_pack(ECHO_RANGE_INVALID - 1u);				/* maximum valid */
    assert(echo_valid(w) && echo_range(w) == ECHO_RANGE_INVALID - 1u);
    assert(!echo_valid(0u));							/* before the first reading */

    uint8_t misses = 0u;
    for (unsigned i = 1u; i < SONAR_NO_ECHO_LIMIT; ++i)
    {
        misses = echo_misses_next(misses, false);
        assert(!echo_no_echo_fault(misses));
    }
    misses = echo_misses_next(misses, false);
    assert(echo_no_echo_fault(misses));					/* limit reached: fault */
    misses = echo_misses_next(misses, false);
    assert(misses == SONAR_NO_ECHO_LIMIT && misses < (1u << ECHO_MISS_BITS));	/* saturates */
    misses = echo_misses_next(misses, true);
    assert(!echo_no_echo_fault(misses) && misses == 0u);			/* an echo clears it */

    puts("echo_range: PASS");
    return 0;
}
