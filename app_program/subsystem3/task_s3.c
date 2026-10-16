#include "task_s3.h"
#include "joint_calibration.h"
#include "../subsystem4/subsystem4.h"
#include "subsystem3_internal.h"
#include "ir_tracking_calibration.h"
#include "drv_ir_tracking.h"
#include "../common/drv_buttons.h"
#include "../common/buzzer.h"
#include "../common/robot_config.h"

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

static unsigned joint_calibration_feedback(unsigned feedback,
                                           calibration_control_t *controls,
                                           IrTracker *tracker,
                                           bool *black_done,
                                           bool *cancel_pending)
{
    if (feedback == 1u && controls->joint_busy)
    {
        *black_done = true;
        feedback = 0u;
    }
    if (feedback == 3u && controls->joint_busy)
    {
        *cancel_pending = true;
        controls->joint_busy = false;
        tracker->calibration = (IrCalibration){0};
    }
    return feedback;
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
    uint32_t now;
    if (!read_time(&now))
    {
        tm_printf((UB *)"[IR] time unavailable; task stopped\n");
        return;
    }
    calibration_control_t controls;
    calibration_control_init(&controls, now);
    uint32_t next = now;
    uint32_t sample_due = now;
    uint32_t sequence = 0u, joint_id = 0u, joint_deadline = 0u;
    bool black_done = false, cancel_pending = false;
    const bool joint_required =
        APP_IR_ONLY_TEST == 0 &&
        robot_config.commissioning == COMMISSIONING_DISABLED;
#if APP_IR_ONLY_TEST
    tm_printf((UB *)"[CAL] IR-only TEST; IMU calibration skipped, "
                    "MOTOR_INHIBITED; "
                    "black=one beep, white=two beeps\n");
#endif
    if (!joint_required && !APP_IR_ONLY_TEST)
    {
        tm_printf((UB *)"[CAL] commissioning IR-only calibration; "
                        "full mission disabled\n");
    }
#if APP_TEST
    uint32_t report_due = now;
#endif
    for (;;)
    {
        if (!read_time(&now))
        {
            tm_printf((UB *)"[IR] time unavailable; task stopped\n");
            barcode_runtime_shutdown();
            return;
        }
        IrCaptureKind before = tracker.capture_kind;
        ir_tracking_check_time(&tracker, now);
        unsigned feedback = calibration_control_feedback(before, &tracker);
        feedback = joint_calibration_feedback(feedback, &controls, &tracker,
                                              &black_done, &cancel_pending);
        request_buzzer_chirps(feedback, now);

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
        TerrainSnapshot terrain = {0};
        bool terrain_copy = s4_get_terrain(&terrain) >= E_OK;
        if (joint_required && tracker.calibration.white_set && terrain_copy &&
            !terrain.calibrated && !controls.joint_busy)
        {
            tracker.calibration = (IrCalibration){0};
        }
        if (controls.joint_busy)
        {
            S4CalibrationStatus status =
                terrain_copy && terrain.calibration.request_id == joint_id
                    ? terrain.calibration.status
                    : S4_CAL_IDLE;
            unsigned joint = joint_calibration_result(
                black_done, inputs.gp21_down, status, now, joint_deadline);
            if (joint != 0u)
            {
                controls.joint_busy = false;
                if (joint == 3u)
                {
                    cancel_pending = true;
                    ir_tracking_cancel_capture(&tracker, IR_CAPTURE_CANCELLED);
                    tracker.calibration = (IrCalibration){0};
                }
                request_buzzer_chirps(joint, now);
                tm_printf((UB *)"[CAL] black+IMU %s id=%u\n",
                          joint == 1u ? "complete" : "failed",
                          (unsigned)joint_id);
            }
        }
        if (cancel_pending && s4_calibration_cancel(joint_id) >= E_OK)
        {
            cancel_pending = false;
        }
        before = tracker.capture_kind;
        unsigned chirps = 0u;
        if (!cancel_pending)
        {
            chirps = calibration_handle_buttons(
                &controls, &tracker, inputs.gp20_down, inputs.gp21_down, now);
        }
        if (joint_required && before == IR_CAPTURE_NONE &&
            tracker.capture_kind == IR_CAPTURE_BLACK)
        {
            if (++joint_id == 0u)
            {
                ++joint_id;
            }
            joint_deadline = now + 5000u;
            black_done = false;
            if (s4_calibration_request(joint_id, joint_deadline) < E_OK)
            {
                ir_tracking_cancel_capture(&tracker, IR_CAPTURE_CANCELLED);
                tracker.calibration = (IrCalibration){0};
                chirps = 3u;
            }
            else
            {
                controls.joint_busy = true;
                tm_printf((UB *)"[CAL] black+IMU started; keep still\n");
            }
        }
        if (chirps == 3u && controls.joint_busy)
        {
            controls.joint_busy = false;
            cancel_pending = true;
            tracker.calibration = (IrCalibration){0};
        }
        request_buzzer_chirps(chirps, now);
        if ((int32_t)(now - sample_due) >= 0)
        {
            IrRaw raw = {0};
            bool acquired = adc_ready && drv_ir_tracking_read(&raw);
            if (!read_time(&now))
            {
                tm_printf((UB *)"[IR] acquisition time unavailable\n");
                barcode_runtime_shutdown();
                return;
            }
            bool advancing = !tracker.have_sample || (now != tracker.sample_ms);
            before = tracker.capture_kind;
            ir_tracking_sample(&tracker, raw, acquired, now);
            if (advancing)
            {
                ++sequence; /* Acquisition attempts, including invalid ones. */
            }
            feedback = calibration_control_feedback(before, &tracker);
            feedback = joint_calibration_feedback(feedback, &controls,
                                                  &tracker, &black_done,
                                                  &cancel_pending);
            request_buzzer_chirps(feedback, now);
            sample_due += IR_SAMPLE_MS;
            if ((int32_t)(now - sample_due) >= 0)
            {
                sample_due = now + IR_SAMPLE_MS;
            }
        }
        /* Service after due ADC work, with a fixed edge budget. GP21 is
           delivered each release; barcode never owns motor/line policy. */
        barcode_runtime_service(inputs.gp21_down);
        if (!buzzer_service(now))
        {
            tm_printf((UB *)"[IR] buzzer service failed; task stopped\n");
            barcode_runtime_shutdown();
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
            report(&tracker, sequence, now, inputs);
            report_due = now + REPORT_PERIOD_MS;
        }
#endif
        next += INPUT_PERIOD_MS;
        if (!read_time(&now))
        {
            tm_printf((UB *)"[IR] scheduling time unavailable\n");
            barcode_runtime_shutdown();
            return;
        }
        if ((int32_t)(now - next) >= 0)
        {
            next = now + INPUT_PERIOD_MS;
        }
        if (tk_dly_tsk((RELTIM)(next - now)) < E_OK)
        {
            tm_printf((UB *)"[IR] task delay failed\n");
            barcode_runtime_shutdown();
            return;
        }
    }
}
