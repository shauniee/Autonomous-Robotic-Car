#include "subsystem3.h"
#include "safety.h"
#include "task_s3.h"
#include "mission.h"
#include <stdatomic.h>
#include "../common/drv_buttons.h"
#include "../common/buzzer.h"
#include "../common/robot_time.h"

// consumers may call before startup, including from the other core
static atomic_int snapshot_mutex;
static S3IrSnapshot latest;

ER subsystem3_init(void)
{
    if (!drv_buttons_init() || !buzzer_init())
    {
        return E_SYS;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT; // set scheduling to priority inheritance
    ID id = tk_cre_mtx(&mutex); // create mutex
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex creation returned=%d\n", id);
#endif
    if (id <= 0) // check if mutex created
    {
        return id < E_OK ? id : E_SYS;
    }
    latest = (S3IrSnapshot){.capture_result = IR_CAPTURE_WAITING};
    atomic_store_explicit(&snapshot_mutex, id, memory_order_release); // ensure setup first before informing mutex is ready
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex published=%d\n", atomic_load_explicit(&snapshot_mutex, memory_order_acquire));
#endif
    return E_OK;
}

ER subsystem3_start(void)
{
    ID mutex_id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex id before task start=%d\n", mutex_id);
#endif
    if (mutex_id <= 0)
    {
        return E_OBJ;
    }
    ID task_id = app_start_task((FP)task_s_task, S3_PRI_LINE, TP_PRC2, "IR line");
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 task start result=%d\n", task_id);
#endif
    if (task_id < E_OK)
    {
        return task_id;
    }
    ER er = safety_start();
    return er < E_OK ? er : mission_start();
}

bool subsystem3_publish(const IrTracker *tracker, uint32_t sequence, uint32_t now)
{
    if (tracker == NULL)
    {
        return false;
    }
    S3IrSnapshot snapshot = ir_snapshot_build(tracker, sequence, now);
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if ((id <= 0) || (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return false;
    }
    latest = snapshot;
    return tk_unl_mtx(id) >= E_OK;
}

bool s3_ir_copy(S3IrSnapshot *snapshot)
{
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if ((snapshot == NULL) || (id <= 0) || (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return false;
    }
    *snapshot = latest;
    if (tk_unl_mtx(id) < E_OK)
    {
        return false;
    }
    ir_snapshot_refresh(snapshot, robot_time_ms());
    return true;
}
