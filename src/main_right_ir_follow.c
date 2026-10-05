/* Right IR edge following; no IMU, ultrasonic, servo or encoder drivers. */
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "robot/right_ir_follow.h"
#include "robot/subsystems.h"
#include "robot/comms_protocol.h"
#include <inttypes.h>
#include <stdio.h>
#define PWM_WRAP 999u
static void brake(void){for(unsigned pin=8;pin<=11;++pin)pwm_set_gpio_level(pin,0);}
static void wheel(unsigned a,unsigned b,float duty,bool forward_a){
    uint16_t level=(uint16_t)(duty*(PWM_WRAP+1));
    pwm_set_gpio_level(a,forward_a?level:0);pwm_set_gpio_level(b,forward_a?0:level);
}
static void init_outputs(void){
    /* GP12 stays LOW: no servo PWM. The freed sensor ports GP2–GP5 are unused. */
    gpio_init(12);gpio_put(12,0);gpio_set_dir(12,GPIO_OUT);
    for(unsigned pin=8;pin<=11;++pin){gpio_init(pin);gpio_put(pin,0);gpio_set_dir(pin,GPIO_OUT);}
    for(unsigned pin=8;pin<=10;pin+=2){
        unsigned slice=pwm_gpio_to_slice_num(pin);pwm_config cfg=pwm_get_default_config();
        pwm_config_set_wrap(&cfg,PWM_WRAP);
        pwm_config_set_clkdiv(&cfg,(float)clock_get_hz(clk_sys)/(20000.0f*(PWM_WRAP+1)));
        pwm_init(slice,&cfg,false);pwm_set_gpio_level(pin,0);pwm_set_gpio_level(pin+1,0);
        gpio_set_function(pin,GPIO_FUNC_PWM);gpio_set_function(pin+1,GPIO_FUNC_PWM);pwm_set_enabled(slice,true);
    }
    brake();
}
int main(void){
    init_outputs();adc_init();adc_gpio_init(FOLLOW_IR_GPIO);adc_select_input(FOLLOW_IR_ADC_CHANNEL);
    gpio_init(20);gpio_set_dir(20,GPIO_IN);gpio_pull_up(20);
    gpio_init(21);gpio_set_dir(21,GPIO_IN);gpio_pull_up(21);
    stdio_init_all();comms_init();watchdog_enable(250,true);
    enum{READY,RUNNING,STOPPED} state=READY;
    const char *reason="calibrate_black_white_via_mqtt";
    const char *calibration_result="waiting";
    uint32_t calibration_id=0;
    bool black_set=false,white_set=false,candidate=false,stable=false,armed=false;
    uint16_t raw=0,black=0,white=0;int calibration_key=0;
    uint32_t last_adc=0,last_publish=0,changed=0;
    FollowResult reading={0};float left=0,right=0;
    static char json[1024];
    puts("right_ir_follow_mqtt: servo disabled. USB b=black,w=white,x=stop; GP20=start/reset, GP21=stop.");
    while(true){
        uint32_t now=to_ms_since_boot(get_absolute_time());
        int key=getchar_timeout_us(0);bool stop=!gpio_get(21)||key=='x';
        if(stop){brake();left=right=0;state=STOPPED;reason="local_stop";}
        if(state!=RUNNING&&(key=='b'||key=='w'))calibration_key=key;
        bool down=!gpio_get(20),pressed=false;
        if(down!=candidate){candidate=down;changed=now;}
        if((uint32_t)(now-changed)>=30){
            if(!candidate)armed=true;
            if(stable!=candidate){stable=candidate;if(stable&&armed){armed=false;pressed=true;}}
        }
        if((uint32_t)(now-last_adc)>=20){
            uint32_t sum=0;for(unsigned i=0;i<8;++i)sum+=adc_read();raw=sum/8;last_adc=now;
            if(state!=RUNNING){
                if(calibration_key=='b'){black=raw;black_set=true;calibration_result="black_captured";}
                if(calibration_key=='w'){white=raw;white_set=true;calibration_result="white_captured";}
            }
            calibration_key=0;
            reading=right_ir_steering(raw,black,white,black_set&&white_set,FOLLOW_EDGE_SIGN);
            if(state==RUNNING&&!reading.valid){
                state=STOPPED;reason="invalid_calibration";
            }
            if(state==RUNNING&&!stop){
                left=reading.left;right=reading.right;
                wheel(8,9,left,FOLLOW_LEFT_FORWARD_A);wheel(10,11,right,FOLLOW_RIGHT_FORWARD_A);
            }else{brake();left=right=0;}
        }
        if(pressed&&!stop){
            if(state==STOPPED){state=READY;reason="ready_press_gp20_to_start";}
            else if(state==READY&&calibration_key==0&&reading.valid){
                state=RUNNING;reason="right_ir_edge_follow";
            }
        }
        (void)comms_poll(now);
        now=to_ms_since_boot(get_absolute_time());
        CalibrationRequest request=comms_take_calibration(now);
        if(request.kind!=CALIBRATION_NONE){
            calibration_id=request.id;
            if(state==RUNNING){calibration_result="rejected_while_moving";}
            else if(calibration_key!=0){calibration_result="rejected_busy";}
            else{calibration_key=request.kind==CALIBRATION_BLACK?'b':'w';calibration_result="pending";}
            last_publish=now-500u;
        }
        if(comms_status_requested()||(uint32_t)(now-last_publish)>=500){
            char fraction[24],error[24];
            if(reading.valid){snprintf(fraction,sizeof(fraction),"%.4f",(double)reading.fraction);snprintf(error,sizeof(error),"%.4f",(double)reading.error);}
            else{snprintf(fraction,sizeof(fraction),"null");snprintf(error,sizeof(error),"null");}
            int n=snprintf(json,sizeof(json),"{\"schema\":3,\"mode\":\"right_ir_follow\",\"state\":\"%s\",\"reason\":\"%s\",\"uptime_ms\":%" PRIu32
                ",\"ir_right\":{\"valid\":true,\"gpio\":27,\"raw_adc\":%u,\"age_ms\":%" PRIu32 ",\"calibrated\":%s,\"black_fraction\":%s,\"edge_error\":%s},"
                "\"motors\":{\"enabled\":%s,\"left_duty\":%.4f,\"right_duty\":%.4f,\"left_mps\":null,\"right_mps\":null},\"servo\":{\"enabled\":false},\"calibration\":{\"request_id\":%" PRIu32 ",\"result\":\"%s\",\"black_reference\":%u,\"white_reference\":%u,\"black_set\":%s,\"white_set\":%s}}",
                state==RUNNING?"FOLLOW_LINE":(state==STOPPED?"STOPPED":"READY"),reason,now,raw,(uint32_t)(now-last_adc),reading.valid?"true":"false",fraction,error,
                left>0||right>0?"true":"false",(double)left,(double)right,calibration_id,calibration_result,black,white,black_set?"true":"false",white_set?"true":"false");
            if(n>0&&(size_t)n<sizeof(json))comms_publish_json(json);
            last_publish=now;
        }
        watchdog_update();sleep_ms(1);
    }
}
