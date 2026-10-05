#include "robot/right_ir_follow.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    FollowResult r=right_ir_steering(2000,3000,1000,true,1);
    assert(r.valid&&fabsf(r.error)<.001f&&r.left==r.right);
    r=right_ir_steering(3000,3000,1000,true,1);
    assert(r.left>r.right&&r.left<=FOLLOW_MAX_DUTY&&r.right>=0);
    r=right_ir_steering(1000,1000,3000,true,1);
    assert(r.valid&&r.fraction==1&&r.left>r.right);
    r=right_ir_steering(3000,3000,1000,true,-1);assert(r.left<r.right);
    assert(!right_ir_steering(2000,2000,2100,true,1).valid);
    assert(!right_ir_steering(2000,3000,1000,false,1).valid);
    assert(!right_ir_steering(2000,3000,1000,true,0).valid);
    assert(!right_ir_steering(5000,3000,1000,true,1).valid);
    puts("Right IR calibration polarity, steering direction and bounded PWM checks passed.");
}
