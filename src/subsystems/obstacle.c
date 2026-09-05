#include "robot/subsystems.h"

void obstacle_init(void) {
    /* TODO Buddy 5: initialise servo and ultrasonic trigger/echo capture. */
}

ObstacleReading obstacle_update(uint32_t now_ms) {
    (void)now_ms;
    /* TODO: nonblocking coarse/fine scan with echo timeout and servo settling.
       Keep forward-range timestamp separate from side-angle measurements.
       A missing echo must not become a fabricated clear-path reading. */
    return (ObstacleReading){0};
}
