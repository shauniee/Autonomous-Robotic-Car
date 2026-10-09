#include "robot_control_policy.h"
#include <math.h>
#include <stddef.h>

static float clamp(float value)
{
    return value < 0.0f ? 0.0f :
        (value > ROBOT_MAX_DUTY ? ROBOT_MAX_DUTY : value);
}

bool robot_policy_moving(const RobotPolicy *policy)
{
    return (policy->state == ROBOT_RUNNING) ||
        (policy->state == ROBOT_RECOVERING);
}

void robot_policy_init(RobotPolicy *policy)
{
    *policy = (RobotPolicy){.state = ROBOT_READY,
        .reason = "capture_black_white"};
}

void robot_policy_stop(RobotPolicy *policy, const char *reason, bool fault)
{
    if ((policy->state != ROBOT_STOPPED) && (policy->state != ROBOT_FAULT))
    {
        policy->request_stop = true;
        policy->cancel_capture = true;
    }
    policy->state = fault ? ROBOT_FAULT : ROBOT_STOPPED;
    policy->reason = reason;
    policy->left_duty = 0.0f;
    policy->right_duty = 0.0f;
    policy->last_direction = 0;
    policy->reacquire_samples = 0u;
    policy->capture_pending = false;
}

static bool fresh(const S3IrSnapshot *ir, uint32_t now)
{
    return (ir != NULL) && ir->have_sample && ir->acquisition_valid &&
        ir->sample_fresh &&
        ((uint32_t)(now - ir->sampled_ms) <= IR_MAX_GAP_MS);
}

static bool start_valid(const S3IrSnapshot *ir, uint32_t now)
{
    return fresh(ir, now) && ir->calibration_valid &&
        ir->normalized_valid && ir->guidance_valid &&
        (ir->capture_kind == IR_CAPTURE_NONE) &&
        (ir->line == IR_LINE_VISIBLE);
}

static bool button_press(RobotPolicy *policy, bool down, bool stop,
    uint32_t now)
{
    if (stop && down)
    {
        policy->button_armed = false;
    }
    if (down != policy->button_candidate)
    {
        policy->button_candidate = down;
        policy->button_changed_ms = now;
    }
    if ((uint32_t)(now - policy->button_changed_ms) < ROBOT_DEBOUNCE_MS)
    {
        return false;
    }
    if (!policy->button_candidate)
    {
        policy->button_armed = true;
    }
    if (policy->button_stable == policy->button_candidate)
    {
        return false;
    }
    policy->button_stable = policy->button_candidate;
    if (!policy->button_stable || !policy->button_armed)
    {
        return false;
    }
    policy->button_armed = false;
    return !stop;
}

static void begin_reset(RobotPolicy *policy, const RobotMotionState *motion,
    uint32_t now, bool start)
{
    if (!motion->available || !motion->driver_ok || !motion->braked ||
        (motion->brake_ack != motion->generation))
    {
        policy->reason = "await_brake_ack";
        return;
    }
    policy->state = start ? ROBOT_START_PENDING : ROBOT_RESET_PENDING;
    policy->reset_generation = motion->generation;
    policy->reset_ms = now;
    policy->request_reset = true;
    policy->reason = start ? "await_start_ack" : "await_reset_ack";
}

static void follow(RobotPolicy *policy, const S3IrSnapshot *ir, uint32_t now)
{
    if (!fresh(ir, now) || !ir->calibration_valid ||
        !ir->normalized_valid || (ir->capture_kind != IR_CAPTURE_NONE))
    {
        robot_policy_stop(policy, "invalid_or_stale_ir", true);
        return;
    }
    bool distinct = !policy->have_ir_sequence ||
        (policy->last_ir_sequence != ir->sequence);
    if (distinct)
    {
        policy->last_ir_sequence = ir->sequence;
        policy->have_ir_sequence = true;
    }
    if ((policy->state == ROBOT_RUNNING) && (ir->line == IR_LINE_LOST))
    {
        if (policy->last_direction == 0)
        {
            robot_policy_stop(policy, "recovery_direction_unknown", false);
            return;
        }
        policy->state = ROBOT_RECOVERING;
        policy->loss_ms = ir->sampled_ms;
        policy->reacquire_samples = 0u;
    }
    if (policy->state == ROBOT_RECOVERING)
    {
        if (ir->line == IR_LINE_BROAD_BLACK)
        {
            policy->reacquire_samples = 0u;
            policy->left_duty = ROBOT_SLOW_DUTY;
            policy->right_duty = ROBOT_SLOW_DUTY;
            policy->reason = "broad_black_slow_straight";
            return; /* Keep the original recovery deadline. */
        }
        bool visible = ir->guidance_valid &&
            (ir->line == IR_LINE_VISIBLE) &&
            ((ir->normalized_left >= ((2.0f * IR_WHITE_EXIT) - 1.0f)) ||
             (ir->normalized_right >= ((2.0f * IR_WHITE_EXIT) - 1.0f)));
        if (!visible)
        {
            policy->reacquire_samples = 0u;
        }
        else if (distinct)
        {
            ++policy->reacquire_samples;
        }
        if (policy->reacquire_samples < 2u)
        {
            policy->left_duty = policy->last_direction > 0 ?
                ROBOT_BASE_DUTY : 0.0f;
            policy->right_duty = policy->last_direction < 0 ?
                ROBOT_BASE_DUTY : 0.0f;
            policy->reason = "line_loss_recovery";
            return;
        }
        policy->state = ROBOT_RUNNING;
        policy->reacquire_samples = 0u;
    }
    if (ir->line == IR_LINE_BROAD_BLACK)
    {
        policy->left_duty = ROBOT_SLOW_DUTY;
        policy->right_duty = ROBOT_SLOW_DUTY;
        policy->reason = "broad_black_slow_straight";
        return;
    }
    if (!ir->guidance_valid || !isfinite(ir->steering) ||
        (ir->steering < -1.0f) || (ir->steering > 1.0f))
    {
        robot_policy_stop(policy, "invalid_steering", true);
        return;
    }
    if (ir->error != 0.0f)
    {
        policy->last_direction = ir->error > 0.0f ? 1 : -1;
    }
    float correction = ir->steering * IR_MAX_CORRECTION;
    policy->left_duty = clamp(ROBOT_BASE_DUTY + correction);
    policy->right_duty = clamp(ROBOT_BASE_DUTY - correction);
    policy->reason = "ir_proportional_follow";
}

void robot_policy_tick(RobotPolicy *policy, const S3IrSnapshot *ir,
    const RobotMotionState *motion, bool start_down, bool stop_down,
    uint32_t now, bool control_due)
{
    policy->request_stop = false;
    policy->request_reset = false;
    policy->cancel_capture = false;
    policy->capture_request = IR_CAPTURE_NONE;
    if (stop_down)
    {
        robot_policy_stop(policy, "local_stop", false);
        if (!policy->stop_previous)
        {
            policy->request_stop = true;
            policy->cancel_capture = true;
        }
    }
    policy->stop_previous = stop_down;
    bool pressed = button_press(policy, start_down, stop_down, now);
    if (stop_down)
    {
        return;
    }
    /* Expiry dominates new data and fresh button inputs. */
    if ((policy->state == ROBOT_RECOVERING) &&
        ((uint32_t)(now - policy->loss_ms) >= ROBOT_LOSS_MS))
    {
        robot_policy_stop(policy, "line_lost", false);
        return;
    }
    bool pending = (policy->state == ROBOT_START_PENDING) ||
        (policy->state == ROBOT_RESET_PENDING);
    if (robot_policy_moving(policy) || pending)
    {
        if (!motion->available || !motion->driver_ok)
        {
            robot_policy_stop(policy, "motion_unavailable", true);
            return;
        }
        if (robot_policy_moving(policy) && motion->stopped)
        {
            robot_policy_stop(policy, "motion_inhibited", true);
            return;
        }
    }
    if (pending)
    {
        bool start = policy->state == ROBOT_START_PENDING;
        if ((motion->generation != policy->reset_generation) ||
            ((uint32_t)(now - policy->reset_ms) > ROBOT_RESET_MS) ||
            (start && !start_valid(ir, now)))
        {
            robot_policy_stop(policy, "reset_or_start_cancelled", true);
            return;
        }
        if (!motion->stopped &&
            (motion->reset_ack == policy->reset_generation))
        {
            policy->state = start ? ROBOT_RUNNING : ROBOT_READY;
            policy->reason = start ? "ir_proportional_follow" :
                "ready_press_gp20_to_start";
            if (start)
            {
                policy->last_direction = 0;
                policy->have_ir_sequence = false;
                follow(policy, ir, now);
            }
        }
        return;
    }
    if (policy->capture_pending && (ir != NULL))
    {
        if ((ir->capture_kind != IR_CAPTURE_NONE) ||
            ((uint32_t)(now - policy->capture_ms) > IR_MAX_GAP_MS))
        {
            policy->capture_pending = false;
        }
    }
    if ((policy->state == ROBOT_READY) && (ir != NULL))
    {
        if (policy->capture_pending || (ir->capture_kind != IR_CAPTURE_NONE))
        {
            policy->reason = ir->capture_kind == IR_CAPTURE_WHITE ?
                "capturing_white" : "capturing_black";
        }
        else if (!ir->calibration.black_set)
        {
            policy->reason = "press_gp20_capture_black";
        }
        else if (!ir->calibration_valid)
        {
            policy->reason = "press_gp20_capture_white";
        }
        else
        {
            policy->reason = "place_on_line_press_gp20";
        }
    }
    if (pressed)
    {
        if ((policy->state == ROBOT_STOPPED) ||
            (policy->state == ROBOT_FAULT))
        {
            begin_reset(policy, motion, now, false);
            return;
        }
        if (policy->state == ROBOT_READY)
        {
            if ((ir == NULL) || !fresh(ir, now))
            {
                policy->reason = "sample_unavailable";
            }
            else if (policy->capture_pending ||
                (ir->capture_kind != IR_CAPTURE_NONE))
            {
                policy->reason = "capture_in_progress";
            }
            else if (!ir->calibration_valid)
            {
                if (!motion->available || !motion->driver_ok ||
                    !motion->braked)
                {
                    policy->reason = "await_brake_ack";
                    return;
                }
                policy->capture_request = ir->calibration.black_set ?
                    IR_CAPTURE_WHITE : IR_CAPTURE_BLACK;
                policy->capture_pending = true;
                policy->capture_ms = now;
                policy->reason = policy->capture_request == IR_CAPTURE_BLACK ?
                    "capturing_black" : "capturing_white";
            }
            else if (!start_valid(ir, now))
            {
                policy->reason = "place_on_line_to_start";
            }
            else if (motion->available && !motion->stopped &&
                motion->driver_ok && motion->braked)
            {
                policy->state = ROBOT_RUNNING;
                policy->last_direction = 0;
                policy->have_ir_sequence = false;
                follow(policy, ir, now);
            }
            else
            {
                begin_reset(policy, motion, now, true);
            }
        }
    }
    if (robot_policy_moving(policy) && control_due)
    {
        follow(policy, ir, now);
    }
}

const char *robot_policy_state_name(RobotState state)
{
    switch (state)
    {
        case ROBOT_READY: return "READY";
        case ROBOT_START_PENDING: return "START_PENDING";
        case ROBOT_RESET_PENDING: return "RESET_PENDING";
        case ROBOT_RUNNING: return "FOLLOW_LINE";
        case ROBOT_RECOVERING: return "RECOVER_LINE";
        case ROBOT_STOPPED: return "STOPPED";
        case ROBOT_FAULT: return "FAULT";
        default: return "FAULT";
    }
}

bool robot_watchdog_progress(RobotWatchdogPolicy *policy, bool valid,
    uint32_t ir_sequence, uint32_t sampled_ms, uint32_t motion_sequence,
    uint32_t serviced_ms, uint32_t now)
{
    if (!valid || (motion_sequence == 0u) ||
        ((uint32_t)(now - sampled_ms) > IR_MAX_GAP_MS) ||
        ((uint32_t)(now - serviced_ms) > IR_MAX_GAP_MS) ||
        (policy->have_progress && ((policy->ir_sequence == ir_sequence) ||
         (policy->motion_sequence == motion_sequence))))
    {
        return false;
    }
    policy->ir_sequence = ir_sequence;
    policy->motion_sequence = motion_sequence;
    policy->have_progress = true;
    return true;
}
