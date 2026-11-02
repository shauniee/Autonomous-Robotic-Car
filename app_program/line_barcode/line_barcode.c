#include "line_barcode.h"
#include "line.h"
#include "../common/robot_config.h"
#include "../motion/motion.h"
#include <stdatomic.h>
#include "../common/buttons.h"
#include "../common/buzzer.h"
#include "../common/robot_time.h"

// consumers may call before startup, including from the other core
static atomic_int snapshot_mutex;
static LineIrSnapshot latest;
#define LINE_COPY_WAIT_MS 2	/* readers' lock wait; matches motion_health() */

ER line_barcode_init(void)
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
    latest = (LineIrSnapshot){0};
    atomic_store_explicit(&snapshot_mutex, id, memory_order_release); // ensure setup first before informing mutex is ready
#if APP_TEST
    tm_printf((UB *)"[TEST] S3 snapshot mutex published=%d\n", atomic_load_explicit(&snapshot_mutex, memory_order_acquire));
#endif
    return E_OK;
}

/* Task-to-publication boundary. A busy mutex skips only this cached copy;
   readers invalidate a stale acquisition by its age. */
static void line_barcode_publish(const LineIrSnapshot *snapshot)
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

#if APP_TEST
static void report(const IrTracker *tracker, uint32_t sequence, uint32_t now,
    ButtonInputs inputs)
{
    LineIrSnapshot snapshot = ir_snapshot_build(tracker, sequence, now);
    tm_printf((UB *)"[IR] seq=%u digital L=%u R=%u acquired=%d fresh=%d "
        "fraction_of_127 L=%u R=%u valid=%d "
        "line=%s age_ms=%u GP20_down=%d GP21_down=%d "
        "SENSOR_ONLY\n",
        (unsigned)sequence, (unsigned)ir_raw_word_raw(snapshot.raw).left,
        (unsigned)ir_raw_word_raw(snapshot.raw).right, ir_raw_word_valid(snapshot.raw),
        snapshot.sample_fresh, (unsigned)snapshot.left_fraction,
        (unsigned)snapshot.right_fraction, snapshot.normalized_valid,
        ir_tracking_line_name(snapshot.line),
        (unsigned)snapshot.sample_age_ms, inputs.gp20_down,
        inputs.gp21_down);
}
#endif

void line_barcode_step(uint32_t now)
{
    static IrTracker tracker;
    static bool initialized;
    static bool ir_ready;
    static uint32_t sample_due;
    static uint32_t sequence;
#if APP_TEST
    static uint32_t report_due;
#endif
    if (!initialized)
    {
        ir_tracking_init(&tracker);
        ir_ready = drv_ir_tracking_init();
        (void)barcode_init();
        sample_due = now;
        initialized = true;
    }
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
    if ((int32_t)(now - sample_due) >= 0)
    {
        IrRaw raw = {0};
        bool acquired = ir_ready && drv_ir_tracking_read(&raw);
        now = robot_time_ms();
        bool advancing = !tracker.have_sample || (now != tracker.sample_ms);
        ir_tracking_sample(&tracker, raw, acquired, now);
        if (advancing)
        {
            ++sequence; /* Acquisition attempts, including invalid ones. */
        }
        sample_due += IR_SAMPLE_MS;
        if ((int32_t)(now - sample_due) >= 0)
        {
            sample_due = now + IR_SAMPLE_MS;
        }
    }
    /* Service after due digital input work, with a fixed edge budget. GP21 is
       delivered each release; barcode never owns motor/line policy. */
    barcode_service(inputs.gp21_down);
    if (!buzzer_service(now))
    {
        tm_printf((UB *)"[IR] buzzer service failed; task stopped\n");
        barcode_service(true);
        (void)motion_stop();
        return;
    }
#if BARCODE_DIAGNOSTICS
    static uint32_t diagnostic_due;
    if ((int32_t)(now-diagnostic_due)>=0) {
        barcode_diagnostic_t record;
        unsigned dropped;
        if (barcode_diagnostic_pop(&record,&dropped))
            tm_printf((UB *)"[BC] L=%u R=%u width=%u result=%u orientation=%u letter=%u action=%u dropped=%u\n",
                record.left,record.right,record.width,record.result,record.orientation,
                record.letter,record.action,dropped);
        diagnostic_due=now+IO_PERIOD_MS;
    }
#endif
    LineIrSnapshot snapshot = ir_snapshot_build(&tracker, sequence, now);
    /* Invalid digital acquisition blocks startup. */
    if (!ir_raw_word_valid(snapshot.raw) || !snapshot.normalized_valid)
    {
        rd_fault_set(RD_OWNER_LINE, RD_FAULT_IR_INVALID);
    }
    else
    {
        rd_fault_clear(RD_OWNER_LINE, RD_FAULT_IR_INVALID);
    }
    line_barcode_publish(&snapshot);
#if APP_TEST
    if ((int32_t)(now - report_due) >= 0)
    {
        report(&tracker, sequence, now, inputs);
        report_due = now + REPORT_PERIOD_MS;
    }
#endif
}



static bool copy_snapshot(LineIrSnapshot *snapshot, TMO wait)
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

bool line_ir_copy(LineIrSnapshot *snapshot)
{
    return copy_snapshot(snapshot, LINE_COPY_WAIT_MS);
}

bool line_ir_try_copy(LineIrSnapshot *snapshot)
{
    return copy_snapshot(snapshot, TMO_POL);
}
