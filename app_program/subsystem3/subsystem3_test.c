#include "subsystem3.h"
#include "ir_tracking.h"
#include "watchdog_policy.h"
#include "../test_report.h"
#if APP_TEST
void subsystem3_test(void) {
    IrCalibration c={{3000,1000},{1000,3000},true,true};
    IrReading r=ir_tracking_reading((IrRaw){1800,2200},&c);
    WatchdogPolicy p={0};
    bool pass=r.valid && r.error==0 && watchdog_progress(&p,true,20,20,1,20) &&
        !watchdog_progress(&p,true,40,20,1,40);
    TEST_INIT();
    TEST_CHECK("S3-U02",pass,"normalization and meaningful progress gate");
    IrTracker tracker;
    ir_tracking_init(&tracker);
    tracker.calibration=c;
    ir_tracking_sample(&tracker,(IrRaw){2600,2800},true,20);
    IrTrackingOutput out=ir_tracking_output(&tracker,20);
    TEST_CHECK("S3-U06",out.valid && out.steering>-.701f && out.steering<-.699f &&
        out.sampled_ms==20 && !ir_tracking_output(&tracker,81).valid,
        "proportional direction and acquisition freshness; no physical motor test");
    TEST_SUMMARY("subsystem3");
}
#endif
