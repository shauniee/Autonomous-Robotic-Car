#include "robot_control.h"
#include "motion_policy.h"
#include "watchdog_policy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static uint32_t now;
static const IrRaw black={3000,1000}, white={1000,3000}, line={1800,2200};
static void ticks(RobotControl *f, bool button, bool stop, IrRaw raw, unsigned count) {
    for (unsigned i=0;i<count;++i) {
        now+=10;
        robot_control_inputs(f,button,stop,now);
        if (now%20==0) robot_control_sample(f,raw,true,now);
        robot_control_check_time(f,now);
    }
}
static void press(RobotControl *f, IrRaw raw) {
    ticks(f,false,false,raw,4); ticks(f,true,false,raw,4);
}
int main(void) {
    RobotControl f; robot_control_init(&f);
    ticks(&f,true,false,black,60); /* Held at boot cannot capture/start. */
    assert(f.tracking.capture_kind==IR_CAPTURE_NONE && f.state==ROBOT_READY);
    press(&f,black); assert(f.tracking.capture_kind==IR_CAPTURE_BLACK);
    ticks(&f,true,false,black,50);
    assert(f.tracking.calibration.black_set && !f.tracking.calibration.white_set && f.chirps==1);
    ticks(&f,true,false,white,60); /* Held capture press does not begin white. */
    assert(f.tracking.capture_kind==IR_CAPTURE_NONE);
    press(&f,white); ticks(&f,true,false,white,50);
    assert(f.tracking.reading.valid && f.chirps==2 && f.state==ROBOT_READY);
    press(&f,white); assert(f.state==ROBOT_READY); /* Both white rejects start. */
    press(&f,black); assert(f.state==ROBOT_READY); /* Both black rejects start. */
    press(&f,line); assert(f.state==ROBOT_RUNNING);
    ticks(&f,true,true,line,1); assert(f.state==ROBOT_STOPPED && f.left_duty==0);
    press(&f,line); assert(f.state==ROBOT_READY && f.tracking.reading.valid);
    ticks(&f,true,false,line,20); assert(f.state==ROBOT_READY);
    press(&f,line); assert(f.state==ROBOT_RUNNING);
    robot_control_stop(&f,"motion_expired");
    assert(f.state==ROBOT_STOPPED && f.right_duty==0);
    robot_control_init(&f);
    press(&f,black); ticks(&f,false,true,black,1);
    assert(f.tracking.capture_kind==IR_CAPTURE_NONE && f.state==ROBOT_STOPPED);
    press(&f,black); assert(f.state==ROBOT_READY); /* Reset only. */
    press(&f,black); ticks(&f,false,false,black,50);
    press(&f,(IrRaw){2900,1100}); ticks(&f,false,false,(IrRaw){2900,1100},50);
    assert(!f.tracking.calibration.black_set && !f.tracking.calibration.white_set && f.chirps==3);
    press(&f,black); assert(f.tracking.capture_kind==IR_CAPTURE_BLACK);
    robot_control_check_time(&f,now+61);
    assert(f.tracking.capture_kind==IR_CAPTURE_NONE);
    MotionRequest r={.1f,.2f,UINT32_MAX-10,true};
    assert(motion_request_valid(r,false,49));
    assert(!motion_request_valid(r,false,50));
    assert(!motion_request_valid(r,true,0));
    r.left=NAN; assert(!motion_request_valid(r,false,0));
    r.left=.36f; assert(!motion_request_valid(r,false,0));
    r.left=-.1f; assert(!motion_request_valid(r,false,0));
    r.left=.1f; r.enabled=false; assert(!motion_request_valid(r,false,0));
    WatchdogPolicy p={0};
    assert(watchdog_progress(&p,true,20,20,1,20));
    assert(!watchdog_progress(&p,true,40,20,1,40)); /* Motion stalled. */
    assert(!watchdog_progress(&p,true,20,40,2,40)); /* Acquisition stalled. */
    assert(!watchdog_progress(&p,false,40,40,2,40));
    assert(!watchdog_progress(&p,true,40,40,2,101));
    assert(watchdog_progress(&p,true,40,40,2,40));
    puts("Button calibration, contrast rejection, stop/reset, motor expiry and watchdog progress checks passed.");
}
