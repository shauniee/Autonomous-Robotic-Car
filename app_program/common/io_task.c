#include "io_task.h"
#include "robot_time.h"
#include "../subsystem1/subsystem1.h"
#include "../subsystem4/subsystem4.h"

static void io_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    uint32_t release = robot_time_ms();
    for (;;)
    {
        uint32_t now = robot_time_ms();
        s4_step(now);
        s1_step(robot_time_ms());
        (void)tk_dly_tsk(robot_time_next_delay(&release, IO_PERIOD_MS,
            robot_time_ms()));
    }
}

ER io_start(void)
{
    ID id = app_start_task((FP)io_task, IO_PRIORITY, TP_PRC1, "I/O");
    return id < E_OK ? id : E_OK;
}
