#include "subsystem3.h"
#include "subsystem3_internal.h"
#include "task_ir.h"
#include "drv_feedback.h"
#include <stdatomic.h>
#if APP_TEST
#include "../../testing/subsystem3/subsystem3_test.h"
#endif

/* Consumers may call before startup, including from the other core. */
static atomic_int snapshot_mutex;
static S3IrSnapshot latest;

ER subsystem3_init(void)
{
    if (!drv_feedback_init())
    {
        return E_SYS;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
    if (id < E_OK)
    {
        return id;
    }
    latest = (S3IrSnapshot){.capture_result = IR_CAPTURE_WAITING};
    atomic_store_explicit(&snapshot_mutex, id, memory_order_release);
    return E_OK;
}

ER subsystem3_start(void)
{
    if (atomic_load_explicit(&snapshot_mutex, memory_order_acquire) <= 0)
    {
        return E_OBJ;
    }
    ID task_id = app_start_task((FP)task_ir_task, S3_PRI_LINE,
        TP_PRC2, "IR line");
    return task_id < E_OK ? task_id : E_OK;
}

bool subsystem3_publish(const IrTracker *tracker, uint32_t sequence,
    uint32_t now)
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
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK)
    {
        return false;
    }
    ir_snapshot_refresh(snapshot, time.lo);
    return true;
}

#if APP_TEST
bool subsystem3_snapshot_self_test(void)
{
    /* Test assertions live in testing/; retain private handle ownership here. */
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    return subsystem3_test_snapshot_boundary(id);
}
#endif
