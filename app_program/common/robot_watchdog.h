/* Optional RP2040 reset watchdog, owned and fed only by the core-0 I/O task.
 * Motion and Mission publish progress after servicing an iteration.
 * This is not a motor-output safety certification; physical reset tests are
 * mandatory before enabling the feature on a driving car.
 */
#ifndef ROBOT_WATCHDOG_H
#define ROBOT_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>
#include "robot_config.h"

/* Kernel-free policy; unsigned subtraction remains valid at uint32 wrap.
 * Zero means no successful producer iteration has been reported.
 */
static inline bool robot_watchdog_progress_fresh(uint32_t now_ms,
    uint32_t motion_ms, uint32_t mission_ms, uint32_t maximum_age_ms)
{
    return motion_ms != 0u && mission_ms != 0u &&
        (uint32_t)(now_ms - motion_ms) <= maximum_age_ms &&
        (uint32_t)(now_ms - mission_ms) <= maximum_age_ms;
}

#if ROBOT_WATCHDOG_ENABLED
void robot_watchdog_note_motion(uint32_t serviced_ms);
void robot_watchdog_note_mission(uint32_t serviced_ms);
void robot_watchdog_io_service(uint32_t now_ms);
#else
static inline void robot_watchdog_note_motion(uint32_t serviced_ms)
{ (void)serviced_ms; }
static inline void robot_watchdog_note_mission(uint32_t serviced_ms)
{ (void)serviced_ms; }
static inline void robot_watchdog_io_service(uint32_t now_ms)
{ (void)now_ms; }
#endif
#endif /* ROBOT_WATCHDOG_H */
