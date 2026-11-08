#include "io_task.h"
#include "robot_time.h"
#include "task_timing.h"
#include "../comms/comms.h"
#include "../terrain/terrain.h"

static void io_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    uint32_t release = robot_time_ms();
    task_timing_t timing = {.window_start_ms = release};
    for (;;)
    {
        uint32_t started_us = task_timing_begin();
        uint32_t now = robot_time_ms();
        terrain_step(now);
        comms_step(robot_time_ms());
        task_timing_report(now);
        uint32_t end_ms = robot_time_ms();
        task_timing_end(&timing, TASK_TIMING_IO, started_us, release,
            IO_PERIOD_MS, end_ms);
        (void)tk_dly_tsk(robot_time_next_delay(&release, IO_PERIOD_MS, end_ms));
    }
}

ER io_start(void)
{
    ID id = app_start_task((FP)io_task, IO_PRIORITY, TP_PRC1, "I/O");
    return id < E_OK ? id : E_OK;
}
