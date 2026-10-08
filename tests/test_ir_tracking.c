#include "ir_tracking.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static const IrRaw black = {3000,1000}, white = {1000,3000};
static bool near(float a, float b) { return fabsf(a-b) < .0001f; }
static void sample(IrTracker *t, IrRaw raw, uint32_t now) {
    ir_tracking_sample(t,raw,true,now);
}
static void capture(IrTracker *t, IrCaptureKind kind, IrRaw raw) {
    uint32_t now=t->sample_ms;
    assert(ir_tracking_capture(t,kind,now));
    for (unsigned n=1;n<25;++n) {
        sample(t,raw,now+n*20);
        assert(t->capture_kind==kind);
    }
    sample(t,raw,now+500);
    assert(t->capture_kind==IR_CAPTURE_NONE && t->capture_count==25);
}
int main(void) {
    IrTracker t; ir_tracking_init(&t);
    assert(!ir_tracking_output(&t,0).valid);
    assert(!ir_tracking_capture(&t,IR_CAPTURE_WHITE,0));
    assert(t.capture_result==IR_CAPTURE_NEED_BLACK);
    assert(!ir_tracking_capture(&t,(IrCaptureKind)9,0));
    assert(t.capture_result==IR_CAPTURE_UNSUPPORTED);
    capture(&t,IR_CAPTURE_BLACK,black);
    assert(!t.reading.valid && t.capture_result==IR_CAPTURE_BLACK_DONE);
    capture(&t,IR_CAPTURE_WHITE,white);
    assert(t.reading.valid && t.capture_result==IR_CAPTURE_WHITE_DONE);

    /* Opposite polarities and independent spans produce matching reflectance. */
    IrCalibration asym={{3000,1000},{1000,4000},true,true};
    IrReading r=ir_tracking_reading((IrRaw){1600,3100},&asym);
    assert(r.valid && near(r.error,0));
    assert(ir_tracking_reading((IrRaw){2200,3100},&asym).error<0);
    assert(ir_tracking_reading((IrRaw){1200,2200},&asym).error>0);
    IrCalibration low=t.calibration;
    low.black.left=low.white.left+299;
    assert(!ir_tracking_reading(black,&low).valid);
    low.black.left=low.white.left+300;
    assert(ir_tracking_reading(black,&low).valid);
    low=t.calibration; low.black.right=low.white.right-299;
    assert(!ir_tracking_reading(black,&low).valid);

    sample(&t,(IrRaw){2600,2800},1020);
    IrTrackingOutput out=ir_tracking_output(&t,1020);
    assert(out.valid && near(out.error,-.7f) && near(out.steering,-.7f));
    assert(out.sampled_ms==1020);
    assert(ir_tracking_output(&t,1080).valid && !ir_tracking_output(&t,1081).valid);
    /* Memoryless P uses new data even at a repeated timestamp; it never reuses D/I. */
    sample(&t,(IrRaw){1200,1400},1020);
    assert(near(ir_tracking_output(&t,1020).steering,.7f));
    sample(&t,(IrRaw){1800,2200},1040);
    assert(near(ir_tracking_output(&t,1040).steering,0));
    sample(&t,(IrRaw){0,0},1060);
    assert(near(ir_tracking_output(&t,1060).steering,1));
    sample(&t,(IrRaw){4095,4095},1080);
    assert(near(ir_tracking_output(&t,1080).steering,-1));

    sample(&t,white,1100);
    assert(t.line==IR_LINE_LOST && !ir_tracking_output(&t,1100).valid);
    sample(&t,(IrRaw){1400,2600},1120); /* .2/.2 remains lost. */
    assert(t.line==IR_LINE_LOST);
    sample(&t,(IrRaw){1500,2600},1140); /* .25/.2 exits hysteresis. */
    assert(t.line==IR_LINE_VISIBLE);
    sample(&t,black,1160);
    assert(t.line==IR_LINE_BROAD_BLACK && !ir_tracking_output(&t,1160).valid);
    ir_tracking_sample(&t,(IrRaw){1800,2200},false,1180);
    assert(!t.sample_valid && t.line==IR_LINE_UNKNOWN);
    ir_tracking_sample(&t,(IrRaw){4096,0},true,1200);
    assert(!t.sample_valid);
    sample(&t,(IrRaw){1800,2200},1220);
    t.reading.error=NAN;
    assert(!ir_tracking_output(&t,1220).valid);
    assert(ir_tracking_classify(t.reading,IR_LINE_VISIBLE)==IR_LINE_UNKNOWN);

    /* Replacing black suppresses old calibration, busy requests cannot change kind. */
    assert(ir_tracking_capture(&t,IR_CAPTURE_BLACK,1220));
    assert(!t.calibration.white_set && !ir_tracking_output(&t,1220).valid);
    assert(!ir_tracking_capture(&t,IR_CAPTURE_WHITE,1221));
    assert(t.capture_kind==IR_CAPTURE_BLACK && t.capture_result==IR_CAPTURE_BUSY);
    ir_tracking_check_time(&t,1281);
    assert(t.capture_kind==IR_CAPTURE_NONE && t.capture_result==IR_CAPTURE_GAP);
    sample(&t,black,1300);
    capture(&t,IR_CAPTURE_BLACK,black);
    capture(&t,IR_CAPTURE_WHITE,(IrRaw){2900,1100});
    assert(t.capture_result==IR_CAPTURE_CONTRAST && !t.calibration.black_set && !t.calibration.white_set);

    ir_tracking_init(&t);
    assert(ir_tracking_capture(&t,IR_CAPTURE_BLACK,0));
    sample(&t,black,20);
    for (unsigned n=0;n<30;++n) sample(&t,black,20);
    assert(t.capture_count==1); /* Replayed acquisition cannot fill a capture window. */
    ir_tracking_check_time(&t,81);
    assert(t.capture_result==IR_CAPTURE_GAP);
    ir_tracking_init(&t);
    assert(ir_tracking_capture(&t,IR_CAPTURE_BLACK,0));
    ir_tracking_check_time(&t,61);
    assert(t.capture_result==IR_CAPTURE_GAP); /* No first acquisition. */

    ir_tracking_init(&t); t.calibration=(IrCalibration){black,white,true,true};
    t.have_sample=true; t.sample_valid=true; t.sample_ms=UINT32_MAX-10;
    sample(&t,(IrRaw){1800,2200},9);
    assert(ir_tracking_output(&t,9).valid && !ir_tracking_output(&t,70).valid);
    puts("[TEST] S3-U06 PASS proportional steering, polarity, saturation and freshness");
    puts("[TEST] S3-U07 PASS calibration outcomes, ADC validity, hysteresis and wraparound");
}
