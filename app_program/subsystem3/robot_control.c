#include "robot_control.h"

static float clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
bool robot_control_is_moving(const RobotControl *f) {
    return f->state == ROBOT_RUNNING || f->state == ROBOT_RECOVERING;
}
void robot_control_stop(RobotControl *f, const char *reason) {
    f->state = ROBOT_STOPPED; f->reason = reason;
    f->left_duty = f->right_duty = f->last_direction = 0;
    f->reacquire_samples = 0;
    ir_tracking_cancel_capture(&f->tracking, IR_CAPTURE_CANCELLED_STOP);
}
void robot_control_init(RobotControl *f) {
    *f = (RobotControl){.state = ROBOT_READY, .reason = "capture_black_white"};
    ir_tracking_init(&f->tracking);
}
bool robot_control_capture(RobotControl *f, IrCaptureKind kind, uint32_t now) {
    if (robot_control_is_moving(f)) {
        f->tracking.capture_result = IR_CAPTURE_MOVING; return false;
    }
    if (f->stop_active) {
        f->tracking.capture_result = IR_CAPTURE_STOP_HELD; return false;
    }
    return ir_tracking_capture(&f->tracking, kind, now);
}
void robot_control_check_time(RobotControl *f, uint32_t now) {
    if (f->state == ROBOT_RECOVERING && (uint32_t)(now - f->loss_ms) >= ROBOT_LOSS_MS)
        robot_control_stop(f, "line_lost");
    if (f->tracking.have_sample && (uint32_t)(now - f->tracking.sample_ms) > IR_MAX_GAP_MS &&
        robot_control_is_moving(f)) robot_control_stop(f, "control_update_late");
    IrCaptureKind before = f->tracking.capture_kind;
    ir_tracking_check_time(&f->tracking, now);
    if (before != IR_CAPTURE_NONE && f->tracking.capture_kind == IR_CAPTURE_NONE) f->chirps = 3;
}
static void recovery_duties(RobotControl *f) {
    f->left_duty = f->last_direction > 0 ? ROBOT_BASE_DUTY : 0;
    f->right_duty = f->last_direction < 0 ? ROBOT_BASE_DUTY : 0;
    f->reason = "line_loss_recovery";
}
void robot_control_sample(RobotControl *f, IrRaw raw, bool acquired, uint32_t now) {
    bool advancing = !f->tracking.have_sample || now != f->tracking.sample_ms;
    robot_control_check_time(f, now); /* New data cannot erase an expired deadline. */
    IrCaptureKind capturing = f->tracking.capture_kind;
    ir_tracking_sample(&f->tracking, raw, acquired, now);
    if (capturing != IR_CAPTURE_NONE && f->tracking.capture_kind == IR_CAPTURE_NONE) {
        switch (f->tracking.capture_result) {
            case IR_CAPTURE_BLACK_DONE:
                f->chirps = 1; f->reason = "press_gp20_capture_white"; break;
            case IR_CAPTURE_WHITE_DONE:
                f->chirps = 2; f->reason = "place_on_line_press_gp20"; break;
            default:
                f->chirps = 3; f->reason = "capture_rejected"; break;
        }
    }
    if (!robot_control_is_moving(f)) return;
    if (!f->tracking.sample_valid) { robot_control_stop(f, "invalid_sample"); return; }
    if (!f->tracking.reading.valid) { robot_control_stop(f, "invalid_calibration"); return; }
    if (f->state == ROBOT_RUNNING && f->tracking.line == IR_LINE_LOST) {
        if (f->last_direction == 0) { robot_control_stop(f, "recovery_direction_unknown"); return; }
        f->state = ROBOT_RECOVERING; f->loss_ms = now; f->reacquire_samples = 0;
    }
    if (f->state == ROBOT_RECOVERING) {
        bool visible = f->tracking.line == IR_LINE_VISIBLE &&
            (f->tracking.reading.left_fraction >= IR_WHITE_EXIT ||
             f->tracking.reading.right_fraction >= IR_WHITE_EXIT);
        if (!visible) f->reacquire_samples = 0;
        else if (advancing) ++f->reacquire_samples;
        if (f->reacquire_samples < ROBOT_REACQUIRE_SAMPLES) {
            recovery_duties(f); return;
        }
        f->state = ROBOT_RUNNING; f->reacquire_samples = 0;
    }
    if (f->tracking.line == IR_LINE_BROAD_BLACK) {
        f->left_duty = f->right_duty = ROBOT_SLOW_DUTY;
        f->reason = "broad_black_slow_straight"; return;
    }
    IrTrackingOutput output = ir_tracking_output(&f->tracking, now);
    if (!output.valid) { robot_control_stop(f, "invalid_steering"); return; }
    if (output.error != 0) f->last_direction = output.error > 0 ? 1 : -1;
    float correction = output.steering * IR_MAX_CORRECTION;
    f->left_duty = clamp(ROBOT_BASE_DUTY + correction, 0, ROBOT_MAX_DUTY);
    f->right_duty = clamp(ROBOT_BASE_DUTY - correction, 0, ROBOT_MAX_DUTY);
    f->reason = "ir_proportional_follow";
}
void robot_control_inputs(RobotControl *f, bool start_down, bool stop, uint32_t now) {
    f->stop_active = stop;
    if (stop) robot_control_stop(f, "local_stop");
    if (start_down != f->button_candidate) {
        f->button_candidate = start_down; f->button_changed_ms = now;
    }
    if ((uint32_t)(now - f->button_changed_ms) < 30u) return;
    if (!f->button_candidate) f->button_armed = true;
    if (f->button_stable == f->button_candidate) return;
    f->button_stable = f->button_candidate;
    if (!f->button_stable || !f->button_armed) return;
    f->button_armed = false;
    if (stop) return;
    if (f->state == ROBOT_STOPPED) {
        f->state = ROBOT_READY; f->reason = "ready_press_gp20_to_start";
    } else if (f->state == ROBOT_READY) {
        if (f->tracking.capture_kind != IR_CAPTURE_NONE) {
            f->reason = "capture_in_progress"; f->chirps = 3; return;
        }
        if (!f->tracking.calibration.black_set || !f->tracking.calibration.white_set) {
            IrCaptureKind kind = f->tracking.calibration.black_set ? IR_CAPTURE_WHITE : IR_CAPTURE_BLACK;
            if (!robot_control_capture(f, kind, now)) f->chirps = 3;
            else f->reason = kind == IR_CAPTURE_BLACK ? "capturing_black" : "capturing_white";
            return;
        }
        if (!f->tracking.have_sample || !f->tracking.sample_valid ||
            (uint32_t)(now - f->tracking.sample_ms) > IR_MAX_GAP_MS || !f->tracking.reading.valid) {
            f->reason = "sample_unavailable"; return;
        }
        if (f->tracking.line != IR_LINE_VISIBLE) { f->reason = "place_on_line_to_start"; return; }
        f->state = ROBOT_RUNNING; f->reason = "ir_proportional_follow";
        f->reacquire_samples = 0; f->last_direction = 0;
    }
}
const char *robot_control_state_name(RobotState state) {
    switch (state) {
        case ROBOT_RUNNING: return "FOLLOW_LINE";
        case ROBOT_RECOVERING: return "RECOVER_LINE";
        case ROBOT_STOPPED: return "STOPPED";
        default: return "READY";
    }
}
