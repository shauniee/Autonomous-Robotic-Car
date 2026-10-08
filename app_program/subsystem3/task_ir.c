/* Nicholas: one RTOS task owns ADC, local inputs, feedback and health policy. */
#include "task_ir.h"
#include "robot_control.h"
#include "drv_ir.h"
#include "drv_feedback.h"
#include "watchdog_policy.h"
#include "../subsystem2/subsystem2.h"

static uint32_t now_ms(void) { SYSTIM t; tk_get_otm(&t); return t.lo; }
void ir_task(INT stacd, void *exinf) {
    (void)stacd; (void)exinf;
    RobotControl car; robot_control_init(&car);
    WatchdogPolicy watchdog = {0};
    drv_feedback_init();
    bool adc_ok = drv_ir_init();
    drv_watchdog_enable();
    uint32_t next = now_ms(), sample_due = next;
#if APP_TEST
    uint32_t report_due = next;
#endif
    for (;;) {
        uint32_t now = now_ms();
        bool stop = !gpio_get_val(21);
        if (stop) s2_stop(); /* Independent of request mutex and capture. */
        MotionHealth h = {0};
        ER health_er = s2_health(&h);
        if (health_er < E_OK || (h.stopped && robot_control_is_moving(&car)))
            robot_control_stop(&car, "motion_inhibited");
        RobotState before = car.state;
        robot_control_inputs(&car, !gpio_get_val(20), stop, now);
        if (before == ROBOT_STOPPED && car.state == ROBOT_READY && !stop) {
            if (s2_reset_stop() < E_OK) robot_control_stop(&car, "reset_failed");
        }
        robot_control_check_time(&car, now);
        if ((int32_t)(now - sample_due) >= 0) {
            IrRaw raw = {0};
            bool acquired = adc_ok && drv_ir_read(&raw);
            robot_control_sample(&car, raw, acquired, now_ms());
            now = now_ms(); sample_due += IR_SAMPLE_MS;
            if ((int32_t)(now - sample_due) >= 0) sample_due = now + IR_SAMPLE_MS;
        }
        if (before == ROBOT_READY && car.state == ROBOT_RUNNING && !stop) {
            if (s2_reset_stop() < E_OK) robot_control_stop(&car, "start_failed");
        }
        if (car.state == ROBOT_STOPPED) s2_stop();
        MotionRequest r = {car.left_duty, car.right_duty, car.tracking.sample_ms,
                           robot_control_is_moving(&car)};
        if (s2_publish(r) < E_OK) {
            robot_control_stop(&car, "publish_failed");
            s2_stop();
        }
        if (car.chirps) { drv_feedback_chirps(car.chirps, now); car.chirps = 0; }
        drv_feedback_service(now);
#if APP_TEST
        if ((int32_t)(now - report_due) >= 0) {
            tm_printf((UB*)"[BENCH] %s reason=%s ADC L=%u R=%u acquired=%d line=%s capture=%s black=%d white=%d MOTOR_INHIBITED\n",
                robot_control_state_name(car.state), car.reason,
                car.tracking.raw.left, car.tracking.raw.right, car.tracking.sample_valid,
                ir_tracking_line_name(car.tracking.line),
                ir_tracking_capture_name(car.tracking.capture_result),
                car.tracking.calibration.black_set, car.tracking.calibration.white_set);
            report_due = now + 500;
        }
#endif
        if (health_er >= E_OK && watchdog_progress(&watchdog,
                car.tracking.have_sample && car.tracking.sample_valid,
                car.tracking.sample_ms, h.serviced_ms, h.sequence, now)) drv_watchdog_feed();
        /* Absolute release: skip missed releases without replaying a backlog. */
        next += 10; now = now_ms();
        if ((int32_t)(now - next) >= 0) next = now + 10;
        tk_dly_tsk((RELTIM)(next - now));
    }
}
