#include "robot_control.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const IrRaw centred={1800,2200}, black={3000,1000}, white={1000,3000};
static const IrRaw left={2600,2800}, right={1200,1400};
static bool near(float a,float b) { return fabsf(a-b)<.0001f; }
static void sample(RobotControl *f,IrRaw raw) {
    robot_control_sample(f,raw,true,f->tracking.sample_ms+20);
}
static void capture(RobotControl *f,IrCaptureKind kind,IrRaw raw) {
    uint32_t now=f->tracking.sample_ms;
    assert(robot_control_capture(f,kind,now));
    for (unsigned n=1;n<=25;++n) robot_control_sample(f,raw,true,now+n*20);
    assert(f->tracking.capture_kind==IR_CAPTURE_NONE);
}
static void calibrated(RobotControl *f) {
    robot_control_init(f);
    capture(f,IR_CAPTURE_BLACK,black); assert(f->chirps==1);
    capture(f,IR_CAPTURE_WHITE,white); assert(f->chirps==2);
    sample(f,centred);
}
static void button(RobotControl *f,bool down,bool stop) {
    for (unsigned n=0;n<3;++n) {
        sample(f,centred);
        robot_control_inputs(f,down,stop,f->tracking.sample_ms);
    }
}
static void start(RobotControl *f) {
    button(f,false,false); button(f,true,false);
    assert(f->state==ROBOT_RUNNING); sample(f,centred);
}
static void prepare(RobotControl *f,IrRaw direction) {
    calibrated(f); start(f); sample(f,direction); sample(f,white);
    assert(f->state==ROBOT_RECOVERING && robot_control_is_moving(f));
}
int main(void) {
    RobotControl f; calibrated(&f); start(&f);
    assert(near(f.left_duty,.1f) && near(f.right_duty,.1f));
    sample(&f,left); assert(near(f.left_duty,.03f) && near(f.right_duty,.17f));
    sample(&f,left); assert(near(f.left_duty,.03f)); /* No derivative history. */
    sample(&f,right); assert(near(f.left_duty,.17f) && near(f.right_duty,.03f));
    sample(&f,(IrRaw){0,0}); assert(near(f.left_duty,.2f) && near(f.right_duty,0));
    sample(&f,(IrRaw){4095,4095}); assert(near(f.left_duty,0) && near(f.right_duty,.2f));
    assert(!robot_control_capture(&f,IR_CAPTURE_BLACK,f.tracking.sample_ms));
    assert(f.tracking.capture_result==IR_CAPTURE_MOVING);
    sample(&f,black);
    assert(f.state==ROBOT_RUNNING && near(f.left_duty,.05f) && near(f.right_duty,.05f));

    /* All nine observation combinations, with the broad-black override. */
    const float bands[]={0,.5f,1};
    for (unsigned l=0;l<3;++l) for (unsigned r=0;r<3;++r) {
        calibrated(&f); start(&f); sample(&f,left);
        IrRaw raw={(uint16_t)(1000+2000*bands[l]),(uint16_t)(3000-2000*bands[r])};
        sample(&f,raw);
        if (l==0 && r==0) assert(f.state==ROBOT_RECOVERING);
        else if (l==2 && r==2) assert(near(f.left_duty,.05f) && near(f.right_duty,.05f));
        else {
            assert(f.state==ROBOT_RUNNING);
            if (l<r) assert(f.left_duty>f.right_duty);
            else if (l>r) assert(f.left_duty<f.right_duty);
            else assert(near(f.left_duty,f.right_duty));
        }
    }
    calibrated(&f); start(&f); sample(&f,white);
    assert(f.state==ROBOT_STOPPED && !strcmp(f.reason,"recovery_direction_unknown"));

    prepare(&f,left); assert(f.left_duty==0 && near(f.right_duty,.1f));
    assert(!robot_control_capture(&f,IR_CAPTURE_WHITE,f.tracking.sample_ms));
    assert(f.tracking.capture_result==IR_CAPTURE_MOVING);
    sample(&f,(IrRaw){1400,2600}); assert(f.tracking.line==IR_LINE_LOST && !f.reacquire_samples);
    sample(&f,centred); assert(f.state==ROBOT_RECOVERING && f.reacquire_samples==1);
    robot_control_sample(&f,centred,true,f.tracking.sample_ms);
    assert(f.state==ROBOT_RECOVERING && f.reacquire_samples==1); /* Same acquisition is not two. */
    sample(&f,centred); assert(f.state==ROBOT_RUNNING && near(f.left_duty,.1f));

    prepare(&f,right); assert(near(f.left_duty,.1f) && f.right_duty==0);
    sample(&f,centred); assert(f.reacquire_samples==1);
    sample(&f,black); assert(f.state==ROBOT_RECOVERING && f.reacquire_samples==0);
    sample(&f,centred); assert(f.state==ROBOT_RECOVERING);
    sample(&f,centred); assert(f.state==ROBOT_RUNNING);

    prepare(&f,right);
    robot_control_inputs(&f,true,true,f.tracking.sample_ms);
    assert(f.state==ROBOT_STOPPED && f.left_duty==0 && f.right_duty==0);
    assert(!strcmp(f.reason,"local_stop")); /* GP21 dominates a recovery turn. */

    prepare(&f,left); uint32_t lost=f.loss_ms;
    for (unsigned n=1;n<5;++n) robot_control_sample(&f,white,true,lost+n*20);
    robot_control_check_time(&f,lost+99);
    assert(f.state==ROBOT_RECOVERING);
    robot_control_sample(&f,centred,true,lost+100);
    assert(f.state==ROBOT_STOPPED && !strcmp(f.reason,"line_lost"));
    assert(f.left_duty==0 && f.right_duty==0);
    button(&f,false,false); button(&f,true,false);
    assert(f.state==ROBOT_READY && f.tracking.calibration.white_set && f.left_duty==0);
    button(&f,true,false); assert(f.state==ROBOT_READY);
    button(&f,false,false); button(&f,true,false); assert(f.state==ROBOT_RUNNING);
    robot_control_inputs(&f,true,true,f.tracking.sample_ms);
    assert(f.state==ROBOT_STOPPED && !strcmp(f.reason,"local_stop"));
    assert(!robot_control_capture(&f,IR_CAPTURE_BLACK,f.tracking.sample_ms));
    assert(f.tracking.capture_result==IR_CAPTURE_STOP_HELD);

    for (unsigned recovering=0;recovering<2;++recovering) {
        calibrated(&f); start(&f);
        if (recovering) { sample(&f,left); sample(&f,white); }
        robot_control_sample(&f,centred,false,f.tracking.sample_ms+20);
        assert(f.state==ROBOT_STOPPED && !strcmp(f.reason,"invalid_sample"));
        assert(f.left_duty==0 && f.right_duty==0);
        calibrated(&f); start(&f);
        if (recovering) { sample(&f,right); sample(&f,white); }
        robot_control_sample(&f,centred,true,f.tracking.sample_ms+61);
        assert(f.state==ROBOT_STOPPED && !strcmp(f.reason,"control_update_late"));
    }

    calibrated(&f); button(&f,true,false); assert(f.state==ROBOT_READY); /* Held at boot. */
    calibrated(&f);
    assert(robot_control_capture(&f,IR_CAPTURE_BLACK,f.tracking.sample_ms));
    robot_control_inputs(&f,false,true,f.tracking.sample_ms+10);
    assert(f.state==ROBOT_STOPPED && f.tracking.capture_result==IR_CAPTURE_CANCELLED_STOP);
    assert(!f.tracking.calibration.white_set);
    robot_control_init(&f);
    assert(robot_control_capture(&f,IR_CAPTURE_BLACK,0));
    robot_control_check_time(&f,61);
    assert(f.chirps==3 && f.tracking.capture_result==IR_CAPTURE_GAP);
    calibrated(&f);
    assert(robot_control_capture(&f,IR_CAPTURE_WHITE,f.tracking.sample_ms));
    robot_control_sample(&f,centred,false,f.tracking.sample_ms+20);
    assert(f.chirps==3 && f.tracking.capture_result==IR_CAPTURE_INVALID_SAMPLE);

    robot_control_init(&f); capture(&f,IR_CAPTURE_BLACK,black);
    assert(robot_control_capture(&f,IR_CAPTURE_WHITE,f.tracking.sample_ms));
    robot_control_inputs(&f,true,true,f.tracking.sample_ms+10);
    assert(f.state==ROBOT_STOPPED && f.tracking.capture_result==IR_CAPTURE_CANCELLED_STOP);
    assert(f.tracking.calibration.black_set && !f.tracking.calibration.white_set);

    calibrated(&f); start(&f);
    f.tracking.sample_ms=UINT32_MAX-40;
    robot_control_sample(&f,left,true,UINT32_MAX-20);
    robot_control_sample(&f,white,true,UINT32_MAX);
    assert(f.state==ROBOT_RECOVERING);
    robot_control_sample(&f,centred,true,19);
    robot_control_sample(&f,centred,true,39);
    assert(f.state==ROBOT_RUNNING);
    puts("[TEST] S3-U08 PASS nine observations, bounded recovery, stop/reset and invalid data");
}
