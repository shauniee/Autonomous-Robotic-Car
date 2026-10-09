#include "subsystem3_test.h"
#include "../../app_program/subsystem3/subsystem3_internal.h"
#include "ir_self_test.h"
#include "../../app_program/subsystem3/drv_ir.h"
#include "../../app_program/subsystem3/drv_feedback.h"
#include "../../app_program/test_report.h"

#if APP_TEST
#define BUTTON_POLL_MS 10u
#define BUTTON_STABLE_POLLS 4u
#define BUTTON_REPORT_POLLS 100u

static ER wait_for_button_state(bool wanted_down, const UB *waiting_message)
{
    unsigned stable_samples = 0u;
    unsigned polls = 0u;
    bool capture_down;
    bool cancel_down;

    while (stable_samples < BUTTON_STABLE_POLLS)
    {
        drv_feedback_inputs(&capture_down, &cancel_down);
        if ((capture_down == wanted_down) && !cancel_down)
        {
            ++stable_samples;
        }
        else
        {
            stable_samples = 0u;
        }

        if ((polls++ % BUTTON_REPORT_POLLS) == 0u)
        {
            tm_printf((UB *)"%s\n", waiting_message);
        }
        if (tk_dly_tsk(BUTTON_POLL_MS) < E_OK)
        {
            tm_printf((UB *)"[TEST] GP20 wait delay failed\n");
            return E_SYS;
        }
    }
    return E_OK;
}

ER subsystem3_test_wait_for_start(void)
{
    tm_printf((UB *)"[TEST] Ready: release GP20, then press and release it to start.\n");
    if (wait_for_button_state(false,
            (const UB *)"[TEST] Waiting for GP20 release...") < E_OK)
    {
        return E_SYS;
    }
    if (wait_for_button_state(true,
            (const UB *)"[TEST] Waiting for GP20 press...") < E_OK)
    {
        return E_SYS;
    }

    tm_printf((UB *)"[TEST] GP20 press accepted; release it to run checks.\n");
    if (wait_for_button_state(false,
            (const UB *)"[TEST] Waiting for GP20 release...") < E_OK)
    {
        return E_SYS;
    }
    tm_printf((UB *)"[TEST] GP20 released; running checks now.\n");
    return E_OK;
}

bool subsystem3_test_snapshot_boundary(ID mutex_id)
{
    /* Exercise the actual copy API without replacing the live snapshot. */
    S3IrSnapshot snapshot;
    bool null_rejected = !s3_ir_copy(NULL);
    bool copied = s3_ir_copy(&snapshot);
    tm_printf((UB *)"[TEST] S3-U12 cache id=%d null=%d copy=%d\n",
        mutex_id, null_rejected, copied);
    if (!null_rejected || !copied)
    {
        return false;
    }
    ER lock_result = mutex_id > 0 ? tk_loc_mtx(mutex_id, TMO_POL) : E_OBJ;
    if (lock_result < E_OK)
    {
        tm_printf((UB *)"[TEST] S3-U12 cache lock failed=%d\n", lock_result);
        return false;
    }
    bool refused = !s3_ir_copy(&snapshot);
    ER unlock_result = tk_unl_mtx(mutex_id);
    tm_printf((UB *)"[TEST] S3-U12 cache refused=%d unlock=%d\n",
        refused, unlock_result);
    return (unlock_result >= E_OK) && refused;
}

void subsystem3_test(void)
{
    bool snapshot_fixtures_pass = ir_self_test_snapshot();
    bool snapshot_boundary_pass = subsystem3_snapshot_self_test();
    TEST_INIT();
    TEST_CHECK("S3-U02", ir_self_test_normalization(),
        "normalization; old watchdog part retired, no hardware claim");
    TEST_CHECK("S3-U06", ir_self_test_guidance(),
        "memoryless P direction, limits, freshness and wraparound");
    TEST_CHECK("S3-U07", ir_self_test_capture(),
        "typed captures, distinct sample span, contrast and cancellation");
    TEST_CHECK("S3-U10", ir_self_test_classification(),
        "nine observation bands, hysteresis and nonfinite rejection");
    TEST_CHECK("S3-U11", ir_self_test_buttons(),
        "calibration debounce, cancellation, replacement and buzz outcomes");
    tm_printf((UB *)"[TEST] S3-U12 details fixtures=%d cache=%d\n",
        snapshot_fixtures_pass, snapshot_boundary_pass);
    TEST_CHECK("S3-U12", snapshot_fixtures_pass && snapshot_boundary_pass,
        "snapshot validity, original time and real nonblocking mutex boundary");
    TEST_CHECK("S3-U13", !drv_ir_read(NULL),
        "null ADC destination rejected; physical protocol test separate");
    TEST_SUMMARY("subsystem3");
}
#endif
