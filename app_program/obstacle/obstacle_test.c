#include "../app.h"
#include "obstacle.h"
#include "../test_report.h"
#if APP_TEST
void obstacle_test(void)
{
    /* Mission pings the sonar every SONAR_REPEAT_MS; wait (bounded) for
       enough pings that the no-echo fault is meaningful. */
    obstacle_range_sample_t front = {0}, side;
    for (int i = 0; i < 100 && front.sequence < SONAR_NO_ECHO_LIMIT; i++)
    {
        (void)tk_dly_tsk(10);
        (void)obstacle_range_try_copy(&front, &side);
    }
    TEST_INIT();
    TEST_CHECK("S5-U01", front.sequence != 0u && echo_valid(front.word) &&
        (rd_fault_get(RD_OWNER_OBSTACLE) & RD_FAULT_NO_ECHO) == 0u,
        "front sonar echo valid (needs a target in range), no NO_ECHO fault");
    TEST_SUMMARY("obstacle");
}
#endif
