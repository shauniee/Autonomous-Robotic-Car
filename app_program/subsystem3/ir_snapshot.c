#include "ir_snapshot.h"
#include <stddef.h>

void ir_snapshot_refresh(S3IrSnapshot *snapshot, uint32_t now)
{
    if (snapshot == NULL)
    {
        return;
    }
    snapshot->sample_age_ms = snapshot->have_sample ?
        (uint32_t)(now - snapshot->sampled_ms) : 0u;
    snapshot->sample_fresh = snapshot->have_sample &&
        snapshot->acquisition_valid &&
        (snapshot->sample_age_ms <= IR_MAX_GAP_MS);
    if (!snapshot->sample_fresh)
    {
        snapshot->normalized_valid = false;
        snapshot->guidance_valid = false;
        snapshot->normalized_left = 0.0f;
        snapshot->normalized_right = 0.0f;
        snapshot->error = 0.0f;
        snapshot->steering = 0.0f;
        snapshot->line = IR_LINE_UNKNOWN;
    }
}

S3IrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now)
{
    S3IrSnapshot snapshot = {0};
    if (tracker == NULL)
    {
        return snapshot;
    }
    snapshot.sequence = sequence;
    snapshot.sampled_ms = tracker->sample_ms;
    snapshot.raw = tracker->raw;
    snapshot.calibration = tracker->calibration;
    snapshot.capture_kind = tracker->capture_kind;
    snapshot.capture_result = tracker->capture_result;
    snapshot.have_sample = tracker->have_sample;
    snapshot.acquisition_valid = tracker->have_sample && tracker->sample_valid;
    snapshot.calibration_valid =
        ir_tracking_reading(tracker->calibration.black,
                            &tracker->calibration).valid;
    IrReading reading = ir_tracking_reading(tracker->raw,
        &tracker->calibration);
    snapshot.normalized_valid = snapshot.acquisition_valid && reading.valid &&
        (tracker->capture_kind == IR_CAPTURE_NONE);
    if (snapshot.normalized_valid)
    {
        snapshot.normalized_left = (2.0f * reading.left_fraction) - 1.0f;
        snapshot.normalized_right = (2.0f * reading.right_fraction) - 1.0f;
        snapshot.error = reading.error;
    }
    IrTrackingOutput output = ir_tracking_output(tracker, now);
    snapshot.guidance_valid = output.valid;
    snapshot.steering = output.steering;
    snapshot.line = output.line;
    ir_snapshot_refresh(&snapshot, now);
    return snapshot;
}
