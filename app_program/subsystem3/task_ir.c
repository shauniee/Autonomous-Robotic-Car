#include "task_ir.h"
#include "subsystem3_internal.h"
#include "calibration_control.h"
#include "drv_ir.h"
#include "drv_feedback.h"

#define INPUT_PERIOD_MS 10u
#define REPORT_PERIOD_MS 500u

static bool read_time(uint32_t *now)
{
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK)
    {
        return false;
    }
    *now = time.lo;
    return true;
}

static void feedback(unsigned chirps, uint32_t now)
{
    if ((chirps > 0u) && !drv_feedback_chirps(chirps, now))
    {
        tm_printf((UB *)"[IR] buzzer request failed\n");
    }
}

#if APP_TEST
static void report(const IrTracker *tracker, uint32_t sequence, uint32_t now)
{
    S3IrSnapshot snapshot = ir_snapshot_build(tracker, sequence, now);
    /* Fixed-point display avoids floating-point console formatting. */
    int left = (int)(snapshot.normalized_left * 1000.0f);
    int right = (int)(snapshot.normalized_right * 1000.0f);
    tm_printf((UB *)"[IR] seq=%u ADC L=%u R=%u acquired=%d fresh=%d "
        "normalized_milli L=%d R=%d valid=%d calibrated=%d "
        "line=%s capture=%s age_ms=%u SENSOR_ONLY\n",
        (unsigned)sequence, (unsigned)snapshot.raw.left,
        (unsigned)snapshot.raw.right, snapshot.acquisition_valid,
        snapshot.sample_fresh, left, right, snapshot.normalized_valid,
        snapshot.calibration_valid, ir_tracking_line_name(snapshot.line),
        ir_tracking_capture_name(snapshot.capture_result),
        (unsigned)snapshot.sample_age_ms);
}
#endif

void task_ir_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    IrTracker tracker;
    ir_tracking_init(&tracker);
    calibration_control_t controls;
    calibration_control_init(&controls);
    bool adc_ready = drv_ir_init();
    uint32_t now;
    if (!read_time(&now))
    {
        tm_printf((UB *)"[IR] time unavailable; task stopped\n");
        return;
    }
    uint32_t next = now;
    uint32_t sample_due = now;
    uint32_t sequence = 0u;
#if APP_TEST
    uint32_t report_due = now;
#endif
    for (;;)
    {
        if (!read_time(&now))
        {
            tm_printf((UB *)"[IR] time unavailable; task stopped\n");
            return;
        }
        IrCaptureKind before = tracker.capture_kind;
        ir_tracking_check_time(&tracker, now);
        feedback(calibration_control_feedback(before, &tracker), now);
        bool capture_down;
        bool cancel_down;
        drv_feedback_inputs(&capture_down, &cancel_down);
        feedback(calibration_control_inputs(&controls, &tracker,
            capture_down, cancel_down, now), now);
        if ((int32_t)(now - sample_due) >= 0)
        {
            IrRaw raw = {0};
            bool acquired = adc_ready && drv_ir_read(&raw);
            if (!read_time(&now))
            {
                tm_printf((UB *)"[IR] acquisition time unavailable\n");
                return;
            }
            bool advancing = !tracker.have_sample || (now != tracker.sample_ms);
            before = tracker.capture_kind;
            ir_tracking_sample(&tracker, raw, acquired, now);
            if (advancing)
            {
                ++sequence; /* Acquisition attempts, including invalid ones. */
            }
            feedback(calibration_control_feedback(before, &tracker), now);
            sample_due += IR_SAMPLE_MS;
            if ((int32_t)(now - sample_due) >= 0)
            {
                sample_due = now + IR_SAMPLE_MS;
            }
        }
        if (!drv_feedback_service(now))
        {
            tm_printf((UB *)"[IR] buzzer service failed; task stopped\n");
            return;
        }
        /* Cache failure skips publication; local work still advances.
           Readers invalidate the cached acquisition after 60 ms. */
        if (!subsystem3_publish(&tracker, sequence, now))
        {
            /* Retry at the next input release, without a busy loop. */
        }
#if APP_TEST
        if ((int32_t)(now - report_due) >= 0)
        {
            report(&tracker, sequence, now);
            report_due = now + REPORT_PERIOD_MS;
        }
#endif
        next += INPUT_PERIOD_MS;
        if (!read_time(&now))
        {
            tm_printf((UB *)"[IR] scheduling time unavailable\n");
            return;
        }
        if ((int32_t)(now - next) >= 0)
        {
            next = now + INPUT_PERIOD_MS;
        }
        if (tk_dly_tsk((RELTIM)(next - now)) < E_OK)
        {
            tm_printf((UB *)"[IR] task delay failed\n");
            return;
        }
    }
}
