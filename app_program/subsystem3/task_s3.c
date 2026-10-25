#include "task_s3.h"
#include "subsystem3_internal.h"
#include "../common/robot_time.h"
#include "ir_tracking_calibration.h"
#include "drv_ir_tracking.h"
#include "../common/drv_buttons.h"
#include "../common/buzzer.h"
#include "../common/robot_config.h"
#include "../subsystem2/subsystem2.h"


static void request_buzzer_chirps(unsigned chirps, uint32_t now)
{
    if (chirps == 0u)
    {
        return;
    }
    if (!buzzer_request(chirps, now))
    {
        tm_printf((UB *)"[IR] buzzer busy or request failed\n");
    }
}

#if APP_TEST
static void report(const IrTracker *tracker, uint32_t sequence, uint32_t now,
    ButtonInputs inputs)
{
    S3IrSnapshot snapshot = ir_snapshot_build(tracker, sequence, now);
    /* Fixed-point display avoids floating-point console formatting. */
    int left = (int)(snapshot.normalized_left * 1000.0f);
    int right = (int)(snapshot.normalized_right * 1000.0f);
    tm_printf((UB *)"[IR] seq=%u ADC L=%u R=%u acquired=%d fresh=%d "
        "normalized_milli L=%d R=%d valid=%d calibrated=%d "
        "line=%s capture=%s age_ms=%u GP20_down=%d GP21_down=%d "
        "SENSOR_ONLY\n",
        (unsigned)sequence, (unsigned)snapshot.raw.left,
        (unsigned)snapshot.raw.right, snapshot.acquisition_valid,
        snapshot.sample_fresh, left, right, snapshot.normalized_valid,
        snapshot.calibration_valid, ir_tracking_line_name(snapshot.line),
        ir_tracking_capture_name(snapshot.capture_result),
        (unsigned)snapshot.sample_age_ms, inputs.gp20_down,
        inputs.gp21_down);
}
#endif

void task_s_task(INT stacd, void *exinf)
{
    (void)stacd; //? might not need to be used
    (void)exinf; //? might not need to be used
    IrTracker tracker;
    ir_tracking_init(&tracker);
    bool adc_ready = drv_ir_tracking_init();
    /* The GPIO dispatcher is core-1 only, so the scanner starts here, disabled. */
    if (!barcode_init()) tm_printf((UB *)"[BARCODE] init failed; scanner unavailable\n");
    uint32_t now;
    now = robot_time_ms();
    calibration_control_t controls;
    calibration_control_init(&controls, now);
    uint32_t next = now;
    uint32_t sample_due = now;
    uint32_t sequence = 0u;
#if APP_TEST
    uint32_t report_due = now;
#endif
    for (;;)
    {
        now = robot_time_ms();
        IrCaptureKind before = tracker.capture_kind;
        ir_tracking_check_time(&tracker, now);
        request_buzzer_chirps(calibration_control_feedback(before, &tracker), now);

        ButtonInputs inputs = drv_buttons_read();
#if APP_TEST
        static bool have_inputs;
        static ButtonInputs previous_inputs;
        if (!have_inputs || inputs.gp20_down != previous_inputs.gp20_down ||
            inputs.gp21_down != previous_inputs.gp21_down)
        {
            tm_printf((UB *)"[IR] buttons GP20_down=%d GP21_down=%d\n",
                inputs.gp20_down, inputs.gp21_down);
            previous_inputs = inputs;
            have_inputs = true;
        }
#endif
        unsigned chirps = calibration_handle_buttons(&controls, &tracker, inputs.gp20_down, inputs.gp21_down, now);
        request_buzzer_chirps(chirps, now);
        if ((int32_t)(now - sample_due) >= 0)
        {
            IrRaw raw = {0};
            bool acquired = adc_ready && drv_ir_tracking_read(&raw);
            now = robot_time_ms();
            bool advancing = !tracker.have_sample || (now != tracker.sample_ms);
            before = tracker.capture_kind;
            ir_tracking_sample(&tracker, raw, acquired, now);
            if (advancing)
            {
                ++sequence; /* Acquisition attempts, including invalid ones. */
            }
            request_buzzer_chirps(calibration_control_feedback(before, &tracker), now);
            sample_due += IR_SAMPLE_MS;
            if ((int32_t)(now - sample_due) >= 0)
            {
                sample_due = now + IR_SAMPLE_MS;
            }
        }
        /* Service after due ADC work, with a fixed edge budget. GP21 is
           delivered each release; barcode never owns motor/line policy. */
        barcode_service(inputs.gp21_down);
        if (!buzzer_service(now))
        {
            tm_printf((UB *)"[IR] buzzer service failed; task stopped\n");
            barcode_stop();
            s2_stop();
            for (;;) tk_slp_tsk(TMO_FEVR); /* Halt: never return from a task */
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
            report(&tracker, sequence, now, inputs);
            report_due = now + REPORT_PERIOD_MS;
        }
#endif
        next += INPUT_PERIOD_MS;
        now = robot_time_ms();
        if ((int32_t)(now - next) >= 0)
        {
            next = now + INPUT_PERIOD_MS;
        }
        if (tk_dly_tsk((RELTIM)(next - now)) < E_OK)
        {
            tm_printf((UB *)"[IR] task delay failed\n");
            barcode_stop();
            s2_stop();
            for (;;) tk_slp_tsk(TMO_FEVR); /* Halt: never return from a task */
        }
    }
}
