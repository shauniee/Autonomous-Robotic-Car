#include "robot_control_task.h"
#include "robot_control.h"
#include "robot_control_policy.h"
#include "subsystem2/subsystem2.h"
#include "subsystem3/subsystem3.h"
#include <stdatomic.h>


#define INPUT_PERIOD_MS 10u
#define CONTROL_PERIOD_MS 20u
#define WATCHDOG_MS 250u
#define WATCHDOG_CONTROL 0x40058000u
#define WATCHDOG_LOAD 0x40058004u
#define WATCHDOG_SCRATCH4 0x4005801cu
#define PSM_WATCHDOG_SELECT 0x40010008u
#define WATCHDOG_ENABLE (1u << 30)
#define WATCHDOG_DEBUG_PAUSE (7u << 24)
#define WATCHDOG_RESET_DOMAINS (0x1ffffu & ~3u)
#define WATCHDOG_LOAD_VALUE (WATCHDOG_MS * 1000u * 2u)

static atomic_int reporting_mutex;
static RobotControlSnapshot latest;
static bool have_report;

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

static void watchdog_enable(void)
{
    /* RP2040-E1 decrements twice per microsecond. TIMER tick is already
       initialized by app_main. Never reset the shared TIMER/PWM blocks. */
    clr_w(WATCHDOG_CONTROL, WATCHDOG_ENABLE | WATCHDOG_DEBUG_PAUSE);
    set_w(PSM_WATCHDOG_SELECT, WATCHDOG_RESET_DOMAINS);
    out_w(WATCHDOG_SCRATCH4, 0u);
    out_w(WATCHDOG_LOAD, WATCHDOG_LOAD_VALUE);
    set_w(WATCHDOG_CONTROL, WATCHDOG_ENABLE);
}

static void label_copy(char *destination, unsigned capacity,
    const char *source)
{
    unsigned i = 0u;
    while ((i + 1u < capacity) && (source[i] != '\0'))
    {
        destination[i] = source[i];
        ++i;
    }
    destination[i] = '\0';
}

static void publish(const RobotPolicy *policy, uint32_t now)
{
    ID id = atomic_load_explicit(&reporting_mutex, memory_order_acquire);
    if ((id <= 0) || (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return; /* Optional reporting never delays stop/control. */
    }
    latest.observed_ms = now;
    ++latest.sequence;
    latest.left_duty = policy->left_duty;
    latest.right_duty = policy->right_duty;
    latest.motion_enabled = robot_policy_moving(policy);
    label_copy(latest.state, sizeof(latest.state),
        robot_policy_state_name(policy->state));
    label_copy(latest.reason, sizeof(latest.reason), policy->reason);
    have_report = true;
    (void)tk_unl_mtx(id);
}

bool robot_control_copy(RobotControlSnapshot *snapshot)
{
    ID id = atomic_load_explicit(&reporting_mutex, memory_order_acquire);
    if ((snapshot == NULL) || (id <= 0) ||
        (tk_loc_mtx(id, TMO_POL) < E_OK))
    {
        return false;
    }
    RobotControlSnapshot copy = latest;
    bool available = have_report;
    if ((tk_unl_mtx(id) < E_OK) || !available)
    {
        return false;
    }
    *snapshot = copy;
    return true;
}

static void control_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    RobotPolicy policy;
    robot_policy_init(&policy);
    S3IrSnapshot ir = {0};
    MotionHealth health = {0};
    bool have_ir = false;
    bool have_health = false;
    RobotWatchdogPolicy watchdog = {0};
    uint32_t now;
    if (!read_time(&now))
    {
        (void)s2_stop();
        return;
    }
    uint32_t next = now;
    uint32_t control_due = now;
    watchdog_enable();
    for (;;)
    {
        bool start_down;
        bool stop_down;
        if (!s3_local_inputs(&start_down, &stop_down) || !read_time(&now))
        {
            (void)s2_stop();
            return; /* Watchdog resets into inhibited startup. */
        }
        /* Raw stop assertion first: independent of all snapshot/mailbox locks. */
        bool direct_stop = stop_down && !policy.stop_previous;
        if (direct_stop)
        {
            (void)s2_stop();
            s3_capture_cancel();
        }
        S3IrSnapshot ir_copy;
        if (s3_ir_copy(&ir_copy))
        {
            ir = ir_copy;
            have_ir = true;
        }
        if (have_ir)
        {
            ir_snapshot_refresh(&ir, now);
        }
        MotionHealth health_copy;
        if (s2_motion_copy(&health_copy))
        {
            health = health_copy;
            have_health = true;
        }
        bool health_fresh = have_health &&
            ((uint32_t)(now - health.serviced_ms) <= IR_MAX_GAP_MS);
        RobotMotionState motion = {
            .available = health_fresh,
            .stopped = health.stopped,
            .braked = health.output_braked,
            .driver_ok = health.driver_ok,
            .generation = health.stop_generation,
            .brake_ack = health.brake_ack_generation,
            .reset_ack = health.reset_ack_generation
        };
        bool due = (int32_t)(now - control_due) >= 0;
        robot_policy_tick(&policy, have_ir ? &ir : NULL, &motion,
            start_down, stop_down, now, due);
        if (policy.request_stop && !direct_stop)
        {
            (void)s2_stop();
        }
        if (policy.cancel_capture && !direct_stop)
        {
            s3_capture_cancel();
        }
        s3_capture_allow(!stop_down && (policy.state == ROBOT_READY) &&
            health_fresh && health.driver_ok && health.output_braked &&
            have_ir && !ir.calibration_valid);
        if ((policy.capture_request != IR_CAPTURE_NONE) &&
            !s3_capture_request(policy.capture_request))
        {
            robot_policy_stop(&policy, "capture_request_failed", true);
            (void)s2_stop();
            s3_capture_cancel();
        }
        if (policy.request_reset &&
            (s2_request_reset(policy.reset_generation) < E_OK))
        {
            robot_policy_stop(&policy, "reset_request_failed", true);
            (void)s2_stop();
        }
        if (due || policy.request_stop || direct_stop)
        {
            MotionRequest request = {
                .left = policy.left_duty,
                .right = policy.right_duty,
                .sampled_ms = have_ir ? ir.sampled_ms : now,
                .enabled = robot_policy_moving(&policy),
                .generation = motion.generation
            };
            if (s2_publish(request) < E_OK)
            {
                robot_policy_stop(&policy, "publish_failed", true);
                (void)s2_stop();
            }
            if (due)
            {
                control_due += CONTROL_PERIOD_MS;
                if ((int32_t)(now - control_due) >= 0)
                {
                    control_due = now + CONTROL_PERIOD_MS;
                }
            }
        }
        publish(&policy, now);
        if (robot_watchdog_progress(&watchdog,
                have_ir && ir.acquisition_valid && ir.sample_fresh &&
                health_fresh && health.driver_ok,
                ir.sequence, ir.sampled_ms, health.sequence,
                health.serviced_ms, now))
        {
            out_w(WATCHDOG_LOAD, WATCHDOG_LOAD_VALUE);
        }
        next += INPUT_PERIOD_MS;
        if (!read_time(&now))
        {
            (void)s2_stop();
            return;
        }
        if ((int32_t)(now - next) >= 0)
        {
            next = now + INPUT_PERIOD_MS;
        }
        if (tk_dly_tsk((RELTIM)(next - now)) < E_OK)
        {
            (void)s2_stop();
            return;
        }
    }
}

ER robot_control_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
    /* This mutex is only for optional reporting; control still runs. */
    if (id > 0)
    {
        atomic_store_explicit(&reporting_mutex, id, memory_order_release);
    }
    return E_OK;
}

ER robot_control_start(void)
{
    ID id = app_start_task((FP)control_task, ROBOT_CONTROL_PRIORITY,
        TP_PRC2, "Mission Safety");
    return id < E_OK ? id : E_OK;
}
