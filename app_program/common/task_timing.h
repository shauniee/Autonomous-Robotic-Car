/*
 *      task_timing.h
 *      Execution-time and stack evidence for the three application tasks.
 *      Built only into ROBOT_TRACE_ENABLED images (test and trial); the course
 *      image compiles every call below to nothing.
 *
 *      One rd_word_t per task, written only by that task:
 *        bits 0..15   longest task body in the last TASK_TIMING_WINDOW_MS, us,
 *                     saturating at 65535
 *        bits 16..31  releases that ran past the next release since boot,
 *                     saturating at 65535
 *      The I/O task reads all three and prints one [TIMING] line per window.
 */

#ifndef TASK_TIMING_H
#define TASK_TIMING_H

#include "robot_data.h"
#include "robot_time.h"

#define TASK_TIMING_WINDOW_MS   5000u
#define TASK_TIMING_MAX_SHIFT   0u
#define TASK_TIMING_MAX_BITS    16u
#define TASK_TIMING_OVR_SHIFT   16u
#define TASK_TIMING_OVR_BITS    16u

typedef enum
{
    TASK_TIMING_MOTION,
    TASK_TIMING_IO,
    TASK_TIMING_MISSION,
    TASK_TIMING_COUNT
} task_timing_id_t;

/* Private to the owning task. */
typedef struct
{
    uint32_t window_start_ms;
    uint16_t window_max_us;
    uint16_t overruns;
} task_timing_t;

extern rd_word_t task_timing_word[TASK_TIMING_COUNT];

static inline uint32_t task_timing_begin(void)
{
    return ROBOT_TRACE_ENABLED ? robot_time_us() : 0u;
}

/* Call after the task body and before robot_time_next_delay(), with the
   release that started this cycle. */
static inline void task_timing_end(task_timing_t *timing, task_timing_id_t task,
    uint32_t start_us, uint32_t release, uint32_t period, uint32_t now_ms)
{
    if (!ROBOT_TRACE_ENABLED)
    {
        return;
    }
    uint32_t body_us = robot_time_elapsed(robot_time_us(), start_us);
    uint32_t top = rd_mask(TASK_TIMING_MAX_BITS);
    if (body_us > top)
    {
        body_us = top;
    }
    if (body_us > timing->window_max_us)
    {
        timing->window_max_us = (uint16_t)body_us;
    }
    if (robot_time_reached(now_ms, release + period) &&
        timing->overruns < rd_mask(TASK_TIMING_OVR_BITS))
    {
        timing->overruns++;
    }
    if (robot_time_elapsed(now_ms, timing->window_start_ms) >=
        TASK_TIMING_WINDOW_MS)
    {
        rd_word_publish(&task_timing_word[task],
            ((uint32_t)timing->window_max_us << TASK_TIMING_MAX_SHIFT) |
            ((uint32_t)timing->overruns << TASK_TIMING_OVR_SHIFT));
        timing->window_max_us = 0u;
        timing->window_start_ms = now_ms;
    }
}

/* Painted user stack for the next task created, or NULL in course images.
   *size receives the stack size to pass with TA_USERBUF. */
void *task_timing_stack(task_timing_id_t task, SZ *size);

/* I/O task only: prints one [TIMING] line per window. */
void task_timing_report(uint32_t now_ms);

#endif /* TASK_TIMING_H */
