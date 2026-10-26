#include "subsystem3.h"
#include "safety.h"
#include "mission.h"
#include "drv_ir_tracking.h"
#include "../common/robot_config.h"
#include "../subsystem2/subsystem2.h"
#include <stdatomic.h>
#include "../common/buttons.h"
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

/* Task-to-publication boundary. Failure skips only the cached copy. */
static bool subsystem3_publish(const IrTracker *tracker, uint32_t sequence, uint32_t now)
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


typedef struct
{
    Button capture_button;
} calibration_control_t;

static unsigned cancel_capture_if_requested(IrTracker *tracker, bool cancel_down)
{
    if (!cancel_down || tracker->capture_kind == IR_CAPTURE_NONE)
    {
        return 0u;
    }

    ir_tracking_cancel_capture(tracker, IR_CAPTURE_CANCELLED);
    return 3u;
}

static ButtonEvent capture_press_received(
    calibration_control_t *control, 
    bool capture_down, 
    bool cancel_down, 
    uint32_t now)
{
    if (cancel_down && capture_down)
    {
        button_disarm(&control->capture_button);
    }

    return button_update(
        &control->capture_button, capture_down, now);
}

static IrCaptureKind next_capture_kind(const IrTracker *tracker)
{
    if (tracker->calibration.black_set && !tracker->calibration.white_set)
    {
        return IR_CAPTURE_WHITE;
    }

    return IR_CAPTURE_BLACK;
}

static void calibration_control_init(
    calibration_control_t *control,
    uint32_t now)
{
    if (control != NULL)
    {
        button_init(&control->capture_button, now);
    }
}

static unsigned calibration_control_feedback(IrCaptureKind before, const IrTracker *tracker)
{
    if ((tracker == NULL) || (before == IR_CAPTURE_NONE) || (tracker->capture_kind != IR_CAPTURE_NONE))
    {
        return 0u;
    }

    switch (tracker->capture_result)
    {
    case IR_CAPTURE_BLACK_DONE:
        return 1u;
    case IR_CAPTURE_WHITE_DONE:
        return 2u;
    default:
        return 3u;
    }
}

static unsigned calibration_handle_buttons(
    calibration_control_t *control,
    IrTracker *tracker,
    bool capture_down,
    bool cancel_down,
    uint32_t now)
{
    if (control == NULL || tracker == NULL)
    {
        return 0u;
    }

    unsigned chirps = cancel_capture_if_requested(tracker, cancel_down);

    ButtonEvent event = capture_press_received(control, capture_down, cancel_down, now);

    bool press_transition = (event == BUTTON_EVENT_PRESSED) || (event == BUTTON_EVENT_IGNORED_PRESS);

    if (cancel_down && press_transition)
    {
        tracker->capture_result = IR_CAPTURE_CANCEL_HELD;
        return 3u;
    }

    if (event != BUTTON_EVENT_PRESSED)
    {
        return chirps;
    }

    /* Once calibrated, GP20 belongs to the bench start/reset controller. */
    if (tracker->calibration.black_set && tracker->calibration.white_set)
    {
        return chirps;
    }

    IrCaptureKind kind = next_capture_kind(tracker);

    if (!ir_tracking_capture(tracker, kind, now))
    {
        return 3u;
    }

    return chirps;
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

static void task_s_task(INT stacd, void *exinf)
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
