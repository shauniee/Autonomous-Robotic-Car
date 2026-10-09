#ifndef DRV_FEEDBACK_H
#define DRV_FEEDBACK_H

#include <stdbool.h>
#include <stdint.h>

bool drv_feedback_init(void);
void drv_feedback_inputs(bool *capture_down, bool *cancel_down);
bool drv_feedback_chirps(unsigned count, uint32_t now);
bool drv_feedback_service(uint32_t now);

#endif
