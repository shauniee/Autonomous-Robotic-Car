#ifndef DRV_FEEDBACK_H
#define DRV_FEEDBACK_H
#include <stdint.h>
#include <stdbool.h>
void drv_feedback_init(void);
void drv_feedback_chirps(unsigned count, uint32_t now);
void drv_feedback_service(uint32_t now);
void drv_watchdog_enable(void);
void drv_watchdog_feed(void);
void drv_timebase_init(void);
#endif
