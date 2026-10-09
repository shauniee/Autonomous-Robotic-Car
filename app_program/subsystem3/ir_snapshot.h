#ifndef IR_SNAPSHOT_H
#define IR_SNAPSHOT_H

#include "ir_tracking.h"

/* Portable data for S1 and future Mission consumers; no wire format implied.
   Invalid normalized/error/steering fields are zero; use validity flags.
   contrast is absolute ADC span, meaningful only with calibration_valid. */
typedef struct
{
    uint32_t sequence;
    uint32_t sampled_ms;
    uint32_t sample_age_ms;
    IrRaw raw;
    IrCalibration calibration;
    IrRaw contrast;
    float normalized_left;
    float normalized_right;
    float error;
    float steering;
    IrLine line;
    IrCaptureKind capture_kind;
    IrCaptureResult capture_result;
    bool have_sample;
    bool acquisition_valid;
    bool calibration_valid;
    bool sample_fresh;
    bool normalized_valid;
    bool guidance_valid;
} S3IrSnapshot;

S3IrSnapshot ir_snapshot_build(const IrTracker *tracker, uint32_t sequence,
    uint32_t now);
/* Recompute age at read time without changing sequence or acquisition time. */
void ir_snapshot_refresh(S3IrSnapshot *snapshot, uint32_t now);

#endif
