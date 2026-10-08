/* Nicholas: paired calibration, reflectance classification and proportional steering. */
#ifndef ROBOT_IR_TRACKING_H
#define ROBOT_IR_TRACKING_H
#include <stdbool.h>
#include <stdint.h>

#define IR_SAMPLE_MS 20u
#define IR_MAX_GAP_MS 60u
#define IR_CAPTURE_MS 500u
#define IR_MIN_CONTRAST 300
#define IR_MAX_CORRECTION .10f
#define IR_KP .10f
#define IR_WHITE_ENTER .15f
#define IR_WHITE_EXIT .25f
#define IR_BROAD_BLACK .85f

typedef struct { uint16_t left, right; } IrRaw;
typedef struct { IrRaw black, white; bool black_set, white_set; } IrCalibration;
typedef enum { IR_LINE_UNKNOWN, IR_LINE_VISIBLE, IR_LINE_LOST, IR_LINE_BROAD_BLACK } IrLine;
typedef struct { bool valid; float left_fraction, right_fraction, error; } IrReading;
typedef enum { IR_CAPTURE_NONE, IR_CAPTURE_BLACK, IR_CAPTURE_WHITE } IrCaptureKind;
typedef enum {
    IR_CAPTURE_WAITING, IR_CAPTURE_PENDING, IR_CAPTURE_BLACK_DONE, IR_CAPTURE_WHITE_DONE,
    IR_CAPTURE_UNSUPPORTED, IR_CAPTURE_BUSY, IR_CAPTURE_MOVING, IR_CAPTURE_STOP_HELD,
    IR_CAPTURE_GAP, IR_CAPTURE_INVALID_SAMPLE, IR_CAPTURE_CONTRAST,
    IR_CAPTURE_NEED_BLACK, IR_CAPTURE_CANCELLED_STOP
} IrCaptureResult;
typedef struct {
    IrLine line;
    IrCalibration calibration;
    IrReading reading;
    IrRaw raw;
    bool sample_valid, have_sample;
    uint32_t sample_ms;
    IrCaptureKind capture_kind;
    uint32_t capture_ms, capture_count, capture_left, capture_right;
    IrCaptureResult capture_result;
} IrTracker;

/* Steering [-1,1], negative left / positive right. valid means fresh calibrated
   VISIBLE steering, not motion permission or measured speed. LOST and BROAD_BLACK
   may be valid acquisitions but have no P steering. */
typedef struct {
    bool valid;
    IrLine line;
    float error, steering;
    uint32_t sampled_ms;
} IrTrackingOutput;

void ir_tracking_init(IrTracker *tracker);
/* Caller permits capture only while stationary and stop is released. */
bool ir_tracking_capture(IrTracker *tracker, IrCaptureKind kind, uint32_t now);
void ir_tracking_cancel_capture(IrTracker *tracker, IrCaptureResult result);
void ir_tracking_check_time(IrTracker *tracker, uint32_t now);
/* acquired is the ADC outcome; now is original acquisition ms. */
void ir_tracking_sample(IrTracker *tracker, IrRaw raw, bool acquired, uint32_t now);
IrReading ir_tracking_reading(IrRaw raw, const IrCalibration *calibration);
IrLine ir_tracking_classify(IrReading reading, IrLine previous);
IrTrackingOutput ir_tracking_output(const IrTracker *tracker, uint32_t now);
const char *ir_tracking_line_name(IrLine line);
const char *ir_tracking_capture_name(IrCaptureResult result);
#endif
