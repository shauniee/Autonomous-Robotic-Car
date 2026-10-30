#include "subsystem3.h"
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
#define S3_COPY_WAIT_MS 2	/* readers' lock wait; matches s2_health() */

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

/* Task-to-publication boundary. A busy mutex skips only this cached copy;
   readers invalidate a stale acquisition by its age. */
static void subsystem3_publish(const S3IrSnapshot *snapshot)
{
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    static unsigned misses;
    if ((id > 0) && (tk_loc_mtx(id, TMO_POL) >= E_OK))
    {
        latest = *snapshot;
        (void)tk_unl_mtx(id);
        if (misses >= 2u) tm_printf((UB *)"[IR] snapshot publish skipped %u times in a row\n", misses);
        misses = 0u;
    }
    else
    {
        ++misses;
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

/* GP20 captures black, then white; GP21 cancels a capture in progress. */
static unsigned calibration_handle_buttons(Button *capture_button, IrTracker *tracker,
    bool capture_down, bool cancel_down, uint32_t now)
{
    unsigned chirps = 0u;
    if (cancel_down && tracker->capture_kind != IR_CAPTURE_NONE)
    {
        ir_tracking_cancel_capture(tracker, IR_CAPTURE_CANCELLED);
        chirps = 3u;
    }
    if (cancel_down && capture_down)
    {
        button_disarm(capture_button);
    }
    ButtonEvent event = button_update(capture_button, capture_down, now);
    if (cancel_down && (event == BUTTON_EVENT_PRESSED || event == BUTTON_EVENT_IGNORED_PRESS))
    {
        tracker->capture_result = IR_CAPTURE_CANCEL_HELD;
        return 3u;
    }
    /* Once calibrated, GP20 belongs to the bench start/reset controller. */
    if (event != BUTTON_EVENT_PRESSED || (tracker->calibration.black_set && tracker->calibration.white_set))
    {
        return chirps;
    }
    IrCaptureKind kind = tracker->calibration.black_set ? IR_CAPTURE_WHITE : IR_CAPTURE_BLACK;
    return ir_tracking_capture(tracker, kind, now) ? chirps : 3u;
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
    tm_printf((UB *)"[IR] seq=%u ADC L=%u R=%u acquired=%d fresh=%d "
        "fraction_of_127 L=%u R=%u valid=%d calibrated=%d "
        "line=%s capture=%s age_ms=%u GP20_down=%d GP21_down=%d "
        "SENSOR_ONLY\n",
        (unsigned)sequence, (unsigned)ir_raw_word_raw(snapshot.raw).left,
        (unsigned)ir_raw_word_raw(snapshot.raw).right, ir_raw_word_valid(snapshot.raw),
        snapshot.sample_fresh, (unsigned)snapshot.left_fraction,
        (unsigned)snapshot.right_fraction, snapshot.normalized_valid,
        snapshot.calibration_valid, ir_tracking_line_name(snapshot.line),
        ir_tracking_capture_name(snapshot.capture_result),
        (unsigned)snapshot.sample_age_ms, inputs.gp20_down,
        inputs.gp21_down);
}
#endif

void s3_step(uint32_t now, bool calibration_enabled)
{
    static IrTracker tracker;
    static Button capture_button;
    static bool initialized;
    static bool adc_ready;
    static uint32_t sample_due;
    static uint32_t sequence;
#if APP_TEST
    static uint32_t report_due;
#endif
    if (!initialized)
    {
        ir_tracking_init(&tracker);
        adc_ready = drv_ir_tracking_init();
        (void)barcode_init();
        button_init(&capture_button, now);
        sample_due = now;
        initialized = true;
    }
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
    unsigned chirps = calibration_enabled ?
        calibration_handle_buttons(&capture_button, &tracker,
            inputs.gp20_down, inputs.gp21_down, now) : 0u;
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
        barcode_service(true);
        (void)s2_stop();
        return;
    }
    S3IrSnapshot snapshot = ir_snapshot_build(&tracker, sequence, now);
    /* Report-only fault (it blocks a start): no reading, or a calibrated
       reading outside the calibrated range. */
    if (!ir_raw_word_valid(snapshot.raw) || (snapshot.calibration_valid && !snapshot.normalized_valid))
    {
        rd_fault_set(RD_OWNER_LINE, RD_FAULT_IR_INVALID);
    }
    else
    {
        rd_fault_clear(RD_OWNER_LINE, RD_FAULT_IR_INVALID);
    }
    subsystem3_publish(&snapshot);
#if APP_TEST
    if ((int32_t)(now - report_due) >= 0)
    {
        report(&tracker, sequence, now, inputs);
        report_due = now + REPORT_PERIOD_MS;
    }
#endif
}



static bool copy_snapshot(S3IrSnapshot *snapshot, TMO wait)
{
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    /* Wait briefly instead of polling: the holder only copies a struct and
       inherits the reader's priority, so a polled miss would be spurious
       and Safety latches a stop on any failed copy. */
    if ((snapshot == NULL) || (id <= 0) || (tk_loc_mtx(id, wait) < E_OK))
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

bool s3_ir_copy(S3IrSnapshot *snapshot)
{
    return copy_snapshot(snapshot, S3_COPY_WAIT_MS);
}

bool s3_ir_try_copy(S3IrSnapshot *snapshot)
{
    return copy_snapshot(snapshot, TMO_POL);
}
