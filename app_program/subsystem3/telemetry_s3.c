#include "telemetry_s3.h"
#include "../subsystem1/telemetry.h"
#include <math.h>

static int16_t thousandths(float value) {
    if (!isfinite(value) || value < -1 || value > 1) return 0;
    return (int16_t)(value * 1000 + (value < 0 ? -.5f : .5f));
}
static void label(char *destination, unsigned capacity, const char *source) {
    unsigned i = 0;
    while (source && *source && i + 1 < capacity) destination[i++] = *source++;
    destination[i] = '\0';
}
void telemetry_s3_update(const RobotControl *f, uint32_t now, bool test_inhibited) {
    const IrTracker *t = &f->tracking;
    const IrCalibration *c = &t->calibration;
    S1Telemetry s = {
        .observed_ms = now, .sampled_ms = t->sample_ms,
        .raw_left = t->raw.left, .raw_right = t->raw.right,
        .black_left = c->black.left, .black_right = c->black.right,
        .white_left = c->white.left, .white_right = c->white.right,
        .have_sample = t->have_sample, .acquired = t->sample_valid,
        .black_set = c->black_set, .white_set = c->white_set,
        .reading_valid = t->sample_valid && t->reading.valid &&
            isfinite(t->reading.left_fraction) && isfinite(t->reading.right_fraction) &&
            isfinite(t->reading.error),
        .left_fraction = thousandths(t->reading.left_fraction),
        .right_fraction = thousandths(t->reading.right_fraction),
        .error = thousandths(t->reading.error),
        .left_duty = thousandths(f->left_duty), .right_duty = thousandths(f->right_duty),
        .motion_enabled = robot_control_is_moving(f), .test_inhibited = test_inhibited
    };
    label(s.state, sizeof(s.state), robot_control_state_name(f->state));
    label(s.line, sizeof(s.line), ir_tracking_line_name(t->line));
    label(s.reason, sizeof(s.reason), f->reason);
    label(s.capture, sizeof(s.capture), ir_tracking_capture_name(t->capture_result));
    (void)s1_telemetry_update(&s);
}
