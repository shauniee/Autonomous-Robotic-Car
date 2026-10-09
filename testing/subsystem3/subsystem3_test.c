#include "subsystem3_test.h"
#include "../../app_program/subsystem3/subsystem3_internal.h"
#include "ir_self_test.h"
#include "../../app_program/subsystem3/drv_ir.h"
#include "../../app_program/test_report.h"

#if APP_TEST
bool subsystem3_test_snapshot_boundary(ID mutex_id)
{
    /* Exercise the actual copy API without replacing the live snapshot. */
    S3IrSnapshot snapshot;
    if (s3_ir_copy(NULL) || !s3_ir_copy(&snapshot))
    {
        return false;
    }
    if ((mutex_id <= 0) || (tk_loc_mtx(mutex_id, TMO_POL) < E_OK))
    {
        return false;
    }
    bool refused = !s3_ir_copy(&snapshot);
    return (tk_unl_mtx(mutex_id) >= E_OK) && refused;
}

void subsystem3_test(void)
{
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
    TEST_CHECK("S3-U12",
        ir_self_test_snapshot() && subsystem3_snapshot_self_test(),
        "snapshot validity, original time and real nonblocking mutex boundary");
    TEST_CHECK("S3-U13", !drv_ir_read(NULL),
        "null ADC destination rejected; physical protocol test separate");
    TEST_SUMMARY("subsystem3");
}
#endif
