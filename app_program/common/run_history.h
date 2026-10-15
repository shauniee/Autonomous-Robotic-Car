#ifndef RUN_HISTORY_H
#define RUN_HISTORY_H
#include <stdbool.h>
#include <stdint.h>
#define RUN_HISTORY_CAPACITY 8u
/* Barcode: TIMER microseconds, signed encoder counts, symbol/action.
   Hump: kernel milliseconds, x/y millimetres, height millimetres. */
typedef struct
{
    uint32_t id, sampled_at, revision;
    int32_t a, b, value;
    unsigned detail;
} RunEvent;
typedef struct
{
    uint32_t run_epoch, count, dropped;
    unsigned length;
    bool overflow;
    RunEvent events[RUN_HISTORY_CAPACITY];
} RunHistory;
void run_history_reset(RunHistory *history, uint32_t epoch);
bool run_history_append(RunHistory *history, RunEvent event);
#endif
