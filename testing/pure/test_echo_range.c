/* Host test for app_program/subsystem5/echo_range.h (issue #48). */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/subsystem5/echo_range.h"

int main(void)
{
    assert(echo_range_mm(SONAR_MIN_PULSE_US - 1u) == ECHO_RANGE_INVALID);	/* too short */
    assert(echo_range_mm(SONAR_TIMEOUT_US) == ECHO_RANGE_INVALID);		/* at the timeout */
    assert(echo_range_mm(0u) == ECHO_RANGE_INVALID);
    uint32_t near_mm = echo_range_mm(SONAR_MIN_PULSE_US);
    assert(near_mm < ECHO_RANGE_INVALID && near_mm > 0u);
    assert(echo_range_mm(5830u) >= 995u && echo_range_mm(5830u) <= 1005u);	/* about 1 m */
    assert(echo_range_mm(SONAR_TIMEOUT_US - 1u) < ECHO_RANGE_INVALID);

    uint32_t w = echo_pack(1000u, 0x12345u);
    assert(echo_valid(w) && echo_range(w) == 1000u && echo_time16(w) == 0x2345u);
    w = echo_pack(ECHO_RANGE_INVALID, 7u);					/* no echo is not clear space */
    assert(!echo_valid(w) && echo_range(w) == ECHO_RANGE_INVALID);
    w = echo_pack(0u, 1u);							/* minimum */
    assert(echo_valid(w) && echo_range(w) == 0u);
    w = echo_pack(ECHO_RANGE_INVALID - 1u, 1u);				/* maximum valid */
    assert(echo_valid(w) && echo_range(w) == ECHO_RANGE_INVALID - 1u);
    assert(!echo_valid(0u));							/* before the first reading */

    uint32_t misses = 0u;
    for (unsigned i = 1u; i < SONAR_NO_ECHO_LIMIT; ++i) assert(!echo_note(&misses, false));
    assert(echo_note(&misses, false));						/* limit reached: fault */
    assert(!echo_note(&misses, true) && misses == 0u);				/* an echo clears it */

    puts("echo_range: PASS");
    return 0;
}
