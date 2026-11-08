#include "task_timing.h"

/* Writer: the task each index names. */
rd_word_t task_timing_word[TASK_TIMING_COUNT];

#if ROBOT_TRACE_ENABLED

/* Same total as the kernel's own allocation: user plus system part. */
#define TASK_TIMING_STACK_BYTES \
    ((TASK_STACK_SIZE + DEFAULT_SYS_STKSZ + 7) / 8 * 8)
#define TASK_TIMING_STACK_WORDS (TASK_TIMING_STACK_BYTES / 4)
#define TASK_TIMING_PAINT       0xA5A5A5A5u

static UW stacks[TASK_TIMING_COUNT][TASK_TIMING_STACK_WORDS]
    __attribute__((aligned(8)));
static const char *const names[TASK_TIMING_COUNT] =
    {"motion", "io", "mission"};

void *task_timing_stack(task_timing_id_t task, SZ *size)
{
    for (unsigned i = 0u; i < TASK_TIMING_STACK_WORDS; i++)
    {
        stacks[task][i] = TASK_TIMING_PAINT;
    }
    *size = (SZ)TASK_TIMING_STACK_BYTES;
    return stacks[task];
}

/* Stacks grow down, so paint left at the low end was never reached. */
static uint32_t stack_used_bytes(task_timing_id_t task)
{
    unsigned untouched = 0u;
    while ((untouched < TASK_TIMING_STACK_WORDS) &&
        (TASK_TIMING_PAINT == stacks[task][untouched]))
    {
        untouched++;
    }
    return (TASK_TIMING_STACK_WORDS - untouched) * 4u;
}

void task_timing_report(uint32_t now_ms)
{
    static uint32_t last_ms;
    if (robot_time_elapsed(now_ms, last_ms) < TASK_TIMING_WINDOW_MS)
    {
        return;
    }
    last_ms = now_ms;
    tm_printf((UB *)"[TIMING] t=%u", (unsigned)now_ms);
    for (unsigned i = 0u; i < TASK_TIMING_COUNT; i++)
    {
        uint32_t word = rd_word_read(&task_timing_word[i]);
        tm_printf((UB *)" | %s max=%uus ovr=%u stack=%u/%uB", names[i],
            (unsigned)rd_field_get(word, TASK_TIMING_MAX_SHIFT,
                TASK_TIMING_MAX_BITS),
            (unsigned)rd_field_get(word, TASK_TIMING_OVR_SHIFT,
                TASK_TIMING_OVR_BITS),
            (unsigned)stack_used_bytes((task_timing_id_t)i),
            (unsigned)TASK_TIMING_STACK_BYTES);
    }
    tm_printf((UB *)"\n");
}

#else

void *task_timing_stack(task_timing_id_t task, SZ *size)
{
    (void)task;
    (void)size;
    return RD_NULL;
}

void task_timing_report(uint32_t now_ms)
{
    (void)now_ms;
}

#endif /* ROBOT_TRACE_ENABLED */
