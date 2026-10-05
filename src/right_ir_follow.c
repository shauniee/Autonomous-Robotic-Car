#include "robot/right_ir_follow.h"
#include <stdlib.h>
static float clamp(float x,float max){return x<0?0:(x>max?max:x);}
FollowResult right_ir_steering(uint16_t raw,uint16_t black,uint16_t white,bool calibrated,int edge){
    FollowResult r={0};int span=(int)black-(int)white;
    if(!calibrated||raw>4095||black>4095||white>4095||abs(span)<FOLLOW_MIN_CONTRAST||(edge!=1&&edge!=-1))return r;
    r.valid=true;r.fraction=clamp(((float)raw-white)/span,1);
    r.error=edge*(2*r.fraction-1);
    r.left=clamp(FOLLOW_BASE_DUTY+FOLLOW_KP*r.error,FOLLOW_MAX_DUTY);
    r.right=clamp(FOLLOW_BASE_DUTY-FOLLOW_KP*r.error,FOLLOW_MAX_DUTY);
    return r;
}
