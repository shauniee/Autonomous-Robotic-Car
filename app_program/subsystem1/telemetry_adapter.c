#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include <math.h>
#include "../float_math.h"
#include "telemetry_adapter.h"
#include "../subsystem3/subsystem3.h"

static bool copy_label(char *out, unsigned capacity, const char *label,
    unsigned source_capacity)
{
    for (unsigned i = 0u; (i < capacity) && (i < source_capacity); ++i)
    {
        out[i] = label[i];
        if (label[i] == '\0')
        {
            return true;
        }
    }
    return false;
}

static bool duty_valid(float duty)
{
    return isfinite(duty) && (duty >= -MOTION_MAX_DUTY) &&
        (duty <= MOTION_MAX_DUTY);
}

static int16_t thousandths(float value)
{
    /* All callers validate finite values within [-1, 1] before narrowing. */
    float rounded = value * 1000.0f + (value < 0.0f ? -0.5f : 0.5f);
    return (int16_t)rounded;
}

bool s1_telemetry_map(const S3IrSnapshot *ir,
    const RobotControlSnapshot *control, const MotionHealth *motion,
    uint32_t now, S1Telemetry *out)
{
    if ((ir == NULL) || (control == NULL) || (out == NULL) ||
        ((uint32_t)(now - control->observed_ms) > IR_MAX_GAP_MS) ||
        !duty_valid(control->left_duty) ||
        !duty_valid(control->right_duty))
    {
        return false;
    }
    S1Telemetry snapshot = {0};
    snapshot.observed_ms = control->observed_ms;
    snapshot.ir_sequence = ir->sequence;
    snapshot.sampled_ms = ir->sampled_ms;
    snapshot.have_sample = ir->have_sample;
    snapshot.acquired = ir->acquisition_valid;
    snapshot.raw_left = ir->raw.left;
    snapshot.raw_right = ir->raw.right;
    snapshot.black_set = ir->calibration.black_set;
    snapshot.white_set = ir->calibration.white_set;
    snapshot.black_left = ir->calibration.black.left;
    snapshot.black_right = ir->calibration.black.right;
    snapshot.white_left = ir->calibration.white.left;
    snapshot.white_right = ir->calibration.white.right;
    snapshot.reading_valid = ir->normalized_valid && ir->sample_fresh;
    if (snapshot.reading_valid)
    {
        if (!isfinite(ir->normalized_left) ||
            !isfinite(ir->normalized_right) || !isfinite(ir->error) ||
            (ir->normalized_left < -1.0f) ||
            (ir->normalized_left > 1.0f) ||
            (ir->normalized_right < -1.0f) ||
            (ir->normalized_right > 1.0f) ||
            (ir->error < -1.0f) || (ir->error > 1.0f))
        {
            return false;
        }
        /* Keep schema 1 fractions: white 0, black 1. */
        snapshot.left_fraction = thousandths(
            (ir->normalized_left + 1.0f) * 0.5f);
        snapshot.right_fraction = thousandths(
            (ir->normalized_right + 1.0f) * 0.5f);
        snapshot.error = thousandths(ir->error);
    }
    snapshot.left_duty = thousandths(control->left_duty);
    snapshot.right_duty = thousandths(control->right_duty);
    snapshot.motion_enabled = control->motion_enabled;
    snapshot.velocity_mode = control->velocity_mode;
    snapshot.speed_target_valid = control->speed_target_valid;
    if (control->speed_target_valid)
    {
        if (!isfinite(control->left_mps) || !isfinite(control->right_mps) ||
            (robot_float_abs(control->left_mps) > 2000000.0f) ||
            (robot_float_abs(control->right_mps) > 2000000.0f))
        {
            return false;
        }
        snapshot.target_left_milli = (int32_t)(control->left_mps * 1000.0f);
        snapshot.target_right_milli = (int32_t)(control->right_mps * 1000.0f);
    }
    snapshot.test_inhibited = APP_TEST != 0;
    if (!copy_label(snapshot.state, sizeof(snapshot.state), control->state,
            sizeof(control->state)) ||
        !copy_label(snapshot.reason, sizeof(snapshot.reason), control->reason,
            sizeof(control->reason)))
    {
        return false;
    }
    const char *line = ir_tracking_line_name(ir->line);
    const char *capture = ir_tracking_capture_name(ir->capture_result);
    if (!copy_label(snapshot.line, sizeof(snapshot.line), line,
            sizeof(snapshot.line)) ||
        !copy_label(snapshot.capture, sizeof(snapshot.capture), capture,
            sizeof(snapshot.capture)))
    {
        return false;
    }
    if ((motion != NULL) && (motion->sequence != 0u) && motion->driver_ok &&
        duty_valid(motion->applied_left) &&
        duty_valid(motion->applied_right))
    {
        snapshot.encoder_valid = motion->encoders.valid;
        snapshot.encoder_left_count = motion->encoders.left_count;
        snapshot.encoder_right_count = motion->encoders.right_count;
        snapshot.encoder_errors = motion->encoders.errors;
        snapshot.action_id = motion->navigation.action_id;
        snapshot.action_state = motion->navigation.action_state;
        snapshot.navigation_sampled_ms = motion->navigation.sampled_ms;
        snapshot.encoder_calibrated = motion->navigation.configured &&
            motion->navigation.encoder_valid &&
            isfinite(motion->navigation.left_mps) &&
            isfinite(motion->navigation.right_mps) &&
            (robot_float_abs(motion->navigation.left_mps) <= 2000000.0f) &&
            (robot_float_abs(motion->navigation.right_mps) <= 2000000.0f);
        if (snapshot.encoder_calibrated)
        {
            snapshot.speed_left_milli =
                (int32_t)(motion->navigation.left_mps * 1000.0f);
            snapshot.speed_right_milli =
                (int32_t)(motion->navigation.right_mps * 1000.0f);
        }
        snapshot.motion_available = true;
        snapshot.motion_serviced_ms = motion->serviced_ms;
        snapshot.motion_stopped = motion->stopped;
        snapshot.motion_braked = motion->output_braked;
        snapshot.applied_left = thousandths(motion->applied_left);
        snapshot.applied_right = thousandths(motion->applied_right);
        snapshot.test_inhibited = motion->test_inhibited;
    }
    *out = snapshot;
    return true;
}

bool s1_telemetry_refresh(uint32_t now)
{
#if TM_MQTT
    S3IrSnapshot ir;
    RobotControlSnapshot control;
    MotionHealth motion;
    S1Telemetry snapshot;
    if (!s3_ir_copy(&ir) || !robot_control_copy(&control))
    {
        return false;
    }
    const MotionHealth *available = s2_motion_copy(&motion) ? &motion : NULL;
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK)
    {
        return false;
    }
    /* A producer on the other core can advance during the copies. Validate
       against collection completion, never an earlier entry timestamp. */
    now = time.lo;
    if (!s1_telemetry_map(&ir, &control, available, now, &snapshot))
    {
        return false;
    }
    S3BarcodeSnapshot barcode;
    if (s3_barcode_copy(&barcode))
    {
        snapshot.barcode_available = true;
        snapshot.barcode_serviced_ms = barcode.serviced_ms;
        snapshot.barcode_sequence = barcode.sequence;
        snapshot.barcode_queued = barcode.queued;
        snapshot.barcode_accepted = barcode.accepted;
        snapshot.barcode_rejected = barcode.rejected;
        snapshot.barcode_discarded = barcode.discarded;
        snapshot.barcode_symbol[0] = barcode.symbol;
        if (!copy_label(snapshot.barcode_state, sizeof(snapshot.barcode_state),
                s3_barcode_state_name(barcode.state),
                sizeof(snapshot.barcode_state)) ||
            !copy_label(snapshot.barcode_result,
                sizeof(snapshot.barcode_result),
                s3_barcode_result_name(barcode.result),
                sizeof(snapshot.barcode_result)))
        {
            return false;
        }
    }
    return s1_telemetry_update(&snapshot);
#else
    (void)now;
    return false;
#endif
}
