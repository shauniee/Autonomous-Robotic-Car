#include "subsystem3.h"
#include "subsystem3_internal.h"
#include "task_ir.h"
#include "drv_feedback.h"
#include <stdatomic.h>
#include <tk/smp.h>
#if APP_TEST
#include "../../testing/subsystem3/subsystem3_test.h"
#endif

/* Consumers may call before startup, including from the other core. */
static atomic_int snapshot_mutex;
static S3IrSnapshot latest;
static UW capture_allowed;
static UW cancel_generation;
static uint32_t serviced_cancel;
static IrCaptureKind pending_capture;
static uint32_t pending_generation;

static UW read_word(UW *word)
{
#if TK_SUPPORT_SMP
    return atomic_cmpxchg(word, 0u, 0u);
#else
    UINT saved;
    DI(saved);
    UW value = *word;
    EI(saved);
    return value;
#endif
}

static void write_word(UW *word, UW value)
{
#if TK_SUPPORT_SMP
    (void)atomic_xchg(word, value);
#else
    UINT saved;
    DI(saved);
    *word = value;
    EI(saved);
#endif
}

ER subsystem3_init(void)
{
    if (!drv_feedback_init())
    {
        return E_SYS;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex creation returned=%d\n", id);
#endif
    if (id <= 0)
    {
        return id < E_OK ? id : E_SYS;
    }
    latest = (S3IrSnapshot){.capture_result = IR_CAPTURE_WAITING};
    atomic_store_explicit(&snapshot_mutex, id, memory_order_release);
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex published=%d\n",
        atomic_load_explicit(&snapshot_mutex, memory_order_acquire));
#endif
    return subsystem3_barcode_init();
}

ER subsystem3_start(void)
{
    ID mutex_id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex id before task start=%d\n",
        mutex_id);
#endif
    if (mutex_id <= 0)
    {
        return E_OBJ;
    }
    ID task_id = app_start_task((FP)task_ir_task, S3_PRI_LINE,
        TP_PRC2, "IR line");
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 task start result=%d\n", task_id);
#endif
    return task_id < E_OK ? task_id : subsystem3_barcode_start();
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

bool s3_local_inputs(bool *start_down, bool *stop_down)
{
    if ((start_down == NULL) || (stop_down == NULL) ||
        (atomic_load_explicit(&snapshot_mutex, memory_order_acquire) <= 0))
    {
        return false;
    }
    drv_feedback_inputs(start_down, stop_down);
    return true;
}

void s3_capture_allow(bool allowed)
{
    write_word(&capture_allowed, allowed ? 1u : 0u);
}

void s3_capture_cancel(void)
{
#if TK_SUPPORT_SMP
    (void)atomic_inc(&cancel_generation);
#else
    UINT saved;
    DI(saved);
    ++cancel_generation;
    EI(saved);
#endif
}

bool s3_capture_request(IrCaptureKind kind)
{
    if (((kind != IR_CAPTURE_BLACK) && (kind != IR_CAPTURE_WHITE)) ||
        (read_word(&capture_allowed) == 0u))
    {
        return false;
    }
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if ((id <= 0) || (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return false;
    }
    bool accepted = pending_capture == IR_CAPTURE_NONE;
    if (accepted)
    {
        pending_capture = kind;
        pending_generation = read_word(&cancel_generation);
    }
    return (tk_unl_mtx(id) >= E_OK) && accepted;
}

void subsystem3_capture_service(IrTracker *tracker, uint32_t now)
{
    uint32_t generation = read_word(&cancel_generation);
    bool changed = generation != serviced_cancel;
    serviced_cancel = generation;
    if (changed || (read_word(&capture_allowed) == 0u))
    {
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_CANCELLED);
    }
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if ((id <= 0) || (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return; /* Request remains pending; retry at next bounded release. */
    }
    IrCaptureKind kind = pending_capture;
    uint32_t requested_generation = pending_generation;
    pending_capture = IR_CAPTURE_NONE;
    if (tk_unl_mtx(id) < E_OK)
    {
        return;
    }
    bool start_down;
    bool stop_down;
    drv_feedback_inputs(&start_down, &stop_down);
    (void)start_down;
    if ((kind != IR_CAPTURE_NONE) && !stop_down &&
        (read_word(&capture_allowed) != 0u) &&
        (requested_generation == read_word(&cancel_generation)))
    {
        (void)ir_tracking_capture(tracker, kind, now);
    }
}
