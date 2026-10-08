/* S3 lifecycle: initialize the timebase, then start one peripheral-owning task. */
#include "subsystem3.h"
#include "task_ir.h"
#include "drv_feedback.h"

ER subsystem3_init(void) { drv_timebase_init(); return E_OK; }
ER subsystem3_start(void) {
    ID tid = app_start_task((FP)ir_task, S3_PRI_BENCH, TP_PRC2, "IR bench");
    return tid < E_OK ? tid : E_OK;
}
