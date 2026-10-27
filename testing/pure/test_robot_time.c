/* Host test for the wrap-safe arithmetic in app_program/common/robot_time_math.h. */
#include <assert.h>
#include <stdio.h>
#include "../../app_program/common/robot_time_math.h"

int main(void)
{
    /* Interval and reach across the 2^32 wrap. */
    const uint32_t starts[] = {0u, 0xFFFFFFF0u, 0xFFFFFFFFu, 0x7FFFFFFFu};
    for (unsigned i = 0; i < sizeof starts / sizeof starts[0]; ++i) {
        for (uint32_t d = 0; d < 1000u; d += 7u) {
            uint32_t then = starts[i];
            uint32_t now = then + d;
            assert(robot_time_elapsed(now, then) == d);
            assert(robot_time_reached(now, then));
            assert(robot_time_reached(now, now));
            assert(!robot_time_reached(then, now) || d == 0u);
        }
    }

    /* Narrow widths: 16-bit and 8-bit stamps wrap correctly when truncated. */
    {
        uint16_t then16 = 0xFFF0u, now16 = (uint16_t)(then16 + 100u);
        assert((uint16_t)(now16 - then16) == 100u);
        uint8_t then8 = 0xF0u, now8 = (uint8_t)(then8 + 40u);
        assert((uint8_t)(now8 - then8) == 40u);
    }

    /* Fixed-rate release across the wrap: 10 ms period straddling 0xFFFFFFFF. */
    uint32_t release = 0xFFFFFFF8u;
    uint32_t now = 0xFFFFFFF8u;
    uint32_t dly = robot_time_next_delay(&release, 10u, now);
    assert(dly == 10u && release == 0xFFFFFFF8u + 10u);
    now = 0xFFFFFFFFu + 3u;                                  /* wraps to 2 */
    dly = robot_time_next_delay(&release, 10u, now);
    assert(dly == release - 2u);
    assert(!robot_time_reached(2u, release) && robot_time_reached(release, 2u));

    /* Overrun re-anchors to now instead of running periods back to back. */
    release = 100u;
    dly = robot_time_next_delay(&release, 10u, 500u);
    assert(dly == 10u && release == 510u);

    /* Loop timing keeps the worst body time and counts overruns, across the
       microsecond wrap. */
    LoopTiming lt = {0};
    const uint32_t bodies[] = {300u, 9000u, 120u, 10000u, 12000u};
    uint32_t us = 0xFFFFF000u;
    for (unsigned i = 0; i < 5u; ++i) {
        loop_timing_begin(&lt, us);
        us += bodies[i];
        loop_timing_end(&lt, us, 10000u);
        us += 10000u;
    }
    assert(lt.max_us == 12000u && lt.overruns == 2u && lt.count == 5u);
    puts("robot_time: PASS");
    return 0;
}
