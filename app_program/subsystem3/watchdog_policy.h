#ifndef WATCHDOG_POLICY_H
#define WATCHDOG_POLICY_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t last_sample, last_motion; bool have_progress; } WatchdogPolicy;
bool watchdog_progress(WatchdogPolicy *policy, bool acquisition_ok, uint32_t sample_ms,
                       uint32_t motion_ms, uint32_t motion_sequence, uint32_t now);
#endif
