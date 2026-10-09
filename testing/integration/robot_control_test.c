#include "../../app_program/robot_control_task.h"
#include "../../app_program/robot_control_policy.h"
#include "../../app_program/subsystem3/subsystem3.h"
#include "../../app_program/subsystem3/subsystem3_internal.h"
#include "../../app_program/test_report.h"

#if APP_TEST
static S3IrSnapshot visible(uint32_t sequence, uint32_t now)
{
    return (S3IrSnapshot){.sequence = sequence, .sampled_ms = now,
        .have_sample = true, .acquisition_valid = true, .sample_fresh = true,
        .calibration_valid = true, .normalized_valid = true,
        .guidance_valid = true, .line = IR_LINE_VISIBLE,
        .normalized_left = 0.0f, .normalized_right = 0.5f,
        .error = 0.25f, .steering = 0.25f};
}

static bool recovery_check(void)
{
    RobotPolicy policy;
    robot_policy_init(&policy);
    policy.state = ROBOT_RUNNING;
    RobotMotionState motion = {.available = true, .driver_ok = true};
    S3IrSnapshot ir = visible(1u, 100u);
    robot_policy_tick(&policy, &ir, &motion, false, false, 100u, true);
    if (!(policy.left_duty > policy.right_duty))
    {
        return false;
    }
    ir.sequence = 2u;
    ir.sampled_ms = 120u;
    ir.line = IR_LINE_LOST;
    ir.guidance_valid = false;
    robot_policy_tick(&policy, &ir, &motion, false, false, 120u, true);
    if ((policy.state != ROBOT_RECOVERING) ||
        (policy.left_duty != ROBOT_BASE_DUTY) ||
        (policy.right_duty != 0.0f))
    {
        return false;
    }
    ir.line = IR_LINE_BROAD_BLACK;
    ir.sequence = 3u;
    ir.sampled_ms = 130u;
    robot_policy_tick(&policy, &ir, &motion, false, false, 130u, true);
    if ((policy.state != ROBOT_RECOVERING) ||
        (policy.left_duty != ROBOT_SLOW_DUTY) ||
        (policy.right_duty != ROBOT_SLOW_DUTY) ||
        (policy.loss_ms != 120u))
    {
        return false;
    }
    ir = visible(4u, 140u);
    robot_policy_tick(&policy, &ir, &motion, false, false, 140u, true);
    robot_policy_tick(&policy, &ir, &motion, false, false, 150u, true);
    if ((policy.state != ROBOT_RECOVERING) ||
        (policy.reacquire_samples != 1u))
    {
        return false;
    }
    ir = visible(5u, 160u);
    robot_policy_tick(&policy, &ir, &motion, false, false, 160u, true);
    if (policy.state != ROBOT_RUNNING)
    {
        return false;
    }
    policy.state = ROBOT_RECOVERING;
    policy.loss_ms = 200u;
    ir = visible(6u, 300u);
    robot_policy_tick(&policy, &ir, &motion, false, false, 300u, true);
    return (policy.state == ROBOT_STOPPED) && policy.request_stop;
}

static bool button_check(void)
{
    RobotPolicy policy;
    robot_policy_init(&policy);
    RobotMotionState motion = {.available = true, .driver_ok = true,
        .stopped = true, .braked = true, .generation = 3u, .brake_ack = 3u};
    S3IrSnapshot ir = visible(1u, 100u);
    robot_policy_tick(&policy, &ir, &motion, false, false, 100u, false);
    robot_policy_tick(&policy, &ir, &motion, true, false, 110u, false);
    ir.sampled_ms = 140u;
    robot_policy_tick(&policy, &ir, &motion, true, false, 140u, false);
    if (!policy.request_reset || (policy.state != ROBOT_START_PENDING))
    {
        return false;
    }
    motion.stopped = false;
    motion.reset_ack = 3u;
    robot_policy_tick(&policy, &ir, &motion, true, false, 150u, false);
    if (!robot_policy_moving(&policy))
    {
        return false;
    }
    robot_policy_tick(&policy, &ir, &motion, true, true, 160u, false);
    if ((policy.state != ROBOT_STOPPED) || !policy.request_stop)
    {
        return false;
    }
    motion.stopped = true;
    motion.generation = 4u;
    motion.brake_ack = 4u;
    robot_policy_tick(&policy, &ir, &motion, false, false, 170u, false);
    robot_policy_tick(&policy, &ir, &motion, false, false, 200u, false);
    robot_policy_tick(&policy, &ir, &motion, true, false, 210u, false);
    robot_policy_tick(&policy, &ir, &motion, true, false, 240u, false);
    if (policy.state != ROBOT_RESET_PENDING)
    {
        return false;
    }
    motion.stopped = false;
    motion.reset_ack = 4u;
    robot_policy_tick(&policy, &ir, &motion, true, false, 250u, false);
    return (policy.state == ROBOT_READY) && !robot_policy_moving(&policy);
}

static bool cancel_check(void)
{
    IrTracker tracker;
    ir_tracking_init(&tracker);
    s3_capture_allow(true);
    bool queued = s3_capture_request(IR_CAPTURE_BLACK);
    s3_capture_cancel();
    subsystem3_capture_service(&tracker, 100u);
    s3_capture_allow(false);
    return queued && (tracker.capture_kind == IR_CAPTURE_NONE);
}

static bool watchdog_check(void)
{
    RobotWatchdogPolicy watchdog = {0};
    bool first = robot_watchdog_progress(&watchdog, true,
        1u, 0xfffffff0u, 1u, 0xfffffff0u, 10u);
    bool duplicate = robot_watchdog_progress(&watchdog, true,
        1u, 10u, 2u, 10u, 10u);
    bool invalid = robot_watchdog_progress(&watchdog, false,
        2u, 10u, 2u, 10u, 10u);
    bool stale = robot_watchdog_progress(&watchdog, true,
        2u, 10u, 2u, 10u, 71u);
    bool next = robot_watchdog_progress(&watchdog, true,
        2u, 70u, 2u, 70u, 71u);
    return first && !duplicate && !invalid && !stale && next;
}

void robot_control_test(void)
{
    TEST_INIT();
    TEST_CHECK("INT-11", recovery_check(),
        "direction, distinct reacquisition and fixed expiry precedence");
    TEST_CHECK("INT-12", button_check(),
        "initial start acknowledgement, stop and separate reset/start");
    TEST_CHECK("INT-13", cancel_check(),
        "cancel invalidates queued capture before consumption");
    TEST_CHECK("INT-14", watchdog_check(),
        "progress/freshness, invalid data and unsigned wrap");
    TEST_SUMMARY("line integration");
}
#endif
