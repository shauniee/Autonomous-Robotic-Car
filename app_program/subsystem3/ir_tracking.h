#ifndef IR_TRACKING_H
#define IR_TRACKING_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

/* RP2040 ADC RESULT is 12 bits: 0 through (2^12 - 1). */

typedef struct
{
    uint16_t left;
    uint16_t right;
} IrRaw;

typedef struct
{
    IrRaw black;
    IrRaw white;
    bool black_set;
    bool white_set;
} IrCalibration;

typedef enum
{
    IR_LINE_UNKNOWN,
    IR_LINE_VISIBLE,
    IR_LINE_LOST,
    IR_LINE_BROAD_BLACK
} IrLine;

typedef struct
{
    bool valid;
    float left_fraction;
    float right_fraction;
    float error;
} IrReading;

typedef enum
{
    IR_CAPTURE_NONE,
    IR_CAPTURE_BLACK,
    IR_CAPTURE_WHITE
} IrCaptureKind;

typedef enum
{
    IR_CAPTURE_WAITING,
    IR_CAPTURE_PENDING,
    IR_CAPTURE_BLACK_DONE,
    IR_CAPTURE_WHITE_DONE,
    IR_CAPTURE_UNSUPPORTED,
    IR_CAPTURE_BUSY,
    IR_CAPTURE_CANCEL_HELD,
    IR_CAPTURE_GAP,
    IR_CAPTURE_INVALID_SAMPLE,
    IR_CAPTURE_CONTRAST,
    IR_CAPTURE_NEED_BLACK,
    IR_CAPTURE_CANCELLED
} IrCaptureResult;

typedef struct
{
    IrLine line;
    IrCalibration calibration;
    IrReading reading;
    IrRaw raw;
    bool sample_valid;
    bool have_sample;
    uint32_t sample_ms;
    IrCaptureKind capture_kind;
    uint32_t capture_requested_ms;
    uint32_t capture_first_ms;
    uint32_t capture_last_ms;
    uint32_t capture_count;
    uint32_t capture_left_sum;
    uint32_t capture_right_sum;
    IrCaptureResult capture_result;
} IrTracker;

/* Valid guidance is fresh calibrated VISIBLE data, never motion permission.
   Negative error/steering means left; positive means right. */
typedef struct
{
    bool valid;
    IrLine line;
    float error;
    float steering;
    uint32_t sampled_ms;
} IrTrackingOutput;

void ir_tracking_init(IrTracker *tracker);
bool ir_tracking_capture(IrTracker *tracker, IrCaptureKind kind, uint32_t now);
void ir_tracking_cancel_capture(IrTracker *tracker, IrCaptureResult result);
void ir_tracking_check_time(IrTracker *tracker, uint32_t now);
/* now is paired acquisition completion time, not publication time. */
void ir_tracking_sample(IrTracker *tracker, IrRaw raw, bool acquired,
                        uint32_t now);
IrReading ir_tracking_reading(IrRaw raw, const IrCalibration *calibration);
IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now);
/* Portable data for S1 and future Mission consumers; no wire format implied.
   Invalid normalized/error/steering fields are zero; use validity flags. */
typedef struct
{
    uint32_t sequence;
    uint32_t sampled_ms;
    uint32_t sample_age_ms;
    IrRaw raw;
    IrCalibration calibration;
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

#if APP_TEST
const char *ir_tracking_line_name(IrLine line);
const char *ir_tracking_capture_name(IrCaptureResult result);
#endif

#endif
