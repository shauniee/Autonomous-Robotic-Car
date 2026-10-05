#include "robot/sensor_bench.h"
#include "fake_bench_sdk.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint32_t now_us, previous_trigger;
static unsigned triggers, servo_pwm;
static bool missing, echo_level;
static uint8_t current_reg, regs[2][256];
static void (*irq)(unsigned,uint32_t);
uint32_t get_absolute_time(void){return now_us;}
uint32_t to_ms_since_boot(uint32_t t){return t/1000;}
uint32_t time_us_32(void){return now_us;}
void busy_wait_us_32(uint32_t us){assert(us<=10);now_us+=us;}
void gpio_init(unsigned pin){(void)pin;}
void gpio_put(unsigned pin,bool level){
    if(pin>=8 && pin<=11)assert(!level);
    if(pin==BENCH_TRIG_GPIO && level){if(triggers)assert(now_us-previous_trigger>=BENCH_TRIGGER_GAP_US);previous_trigger=now_us;++triggers;}
}
bool gpio_get(unsigned pin){assert(pin==BENCH_ECHO_GPIO);return echo_level;}
void gpio_set_dir(unsigned pin,bool out){(void)pin;(void)out;}
void gpio_set_function(unsigned pin,unsigned function){assert(!(pin>=8 && pin<=11));assert(function==GPIO_FUNC_I2C || (pin==12 && function==GPIO_FUNC_PWM));}
void gpio_pull_up(unsigned pin){(void)pin;}
void gpio_pull_down(unsigned pin){(void)pin;}
void gpio_set_irq_enabled_with_callback(unsigned pin,uint32_t events,bool enabled,void (*callback)(unsigned,uint32_t)){assert(pin==2&&events==12&&enabled);irq=callback;}
void adc_init(void){}
void adc_gpio_init(unsigned pin){assert(pin==26);}
void adc_select_input(unsigned input){assert(input==0);}
uint16_t adc_read(void){return 2048;}
void i2c_init(void *bus,unsigned rate){(void)bus;assert(rate==100000);}
int i2c_write_timeout_us(void *bus,uint8_t addr,const uint8_t *bytes,size_t count,bool nostop,unsigned timeout){
    (void)bus;(void)nostop;assert(timeout==2000);if(missing)return -1;
    assert(addr==0x19 || addr==0x1e);current_reg=bytes[0];
    if(count==2)regs[addr==0x19?0:1][bytes[0]]=bytes[1];
    return (int)count;
}
int i2c_read_timeout_us(void *bus,uint8_t addr,uint8_t *bytes,size_t count,bool nostop,unsigned timeout){
    (void)bus;(void)nostop;(void)timeout;if(missing)return -1;
    memset(bytes,0,count);
    if(addr==0x19 && current_reg==0x23)bytes[0]=regs[0][0x23];
    if(addr==0x19 && current_reg==0x27)bytes[0]=8;
    if(addr==0x19 && current_reg==0xa8){assert(count==6);bytes[4]=0x80;bytes[5]=0x3e;}
    if(addr==0x1e && current_reg==0x0a){assert(count==3);bytes[0]=0x48;bytes[1]=0x34;bytes[2]=0x33;}
    if(addr==0x1e && current_reg==0x09)bytes[0]=1;
    if(addr==0x1e && current_reg==0x03){assert(count==6);bytes[0]=4;bytes[1]=0x4c;}
    return (int)count;
}
unsigned pwm_gpio_to_slice_num(unsigned pin){assert(pin==12);return 6;}
pwm_config pwm_get_default_config(void){return (pwm_config){0};}
void pwm_config_set_clkdiv(pwm_config *cfg,float div){cfg->divider=div;assert(div==125);}
void pwm_config_set_wrap(pwm_config *cfg,unsigned wrap){cfg->wrap=wrap;assert(wrap==19999);}
void pwm_init(unsigned slice,const pwm_config *cfg,bool enabled){(void)cfg;assert(slice==6&&!enabled);}
void pwm_set_gpio_level(unsigned pin,uint16_t level){assert(pin==12);servo_pwm=level;}
void pwm_set_enabled(unsigned slice,bool enabled){assert(slice==6&&enabled);}
unsigned clock_get_hz(unsigned clock){(void)clock;return 125000000;}
uint32_t save_and_disable_interrupts(void){return 0;}
void restore_interrupts(uint32_t state){(void)state;}
static void tick(SensorBench *s,uint32_t ms){now_us=ms*1000u;sensor_bench_update(s,ms);}
int main(void){
    SensorBench s;sensor_bench_init(&s);
    assert(s.accel_configured&&s.mag_configured&&servo_pwm==1500&&!s.scan_enabled);
    assert(regs[0][0x20]==0x47&&regs[0][0x23]==0x88&&regs[1][0x02]==0);
    tick(&s,20);assert(s.ir_raw==2048&&s.accel.z==1&&s.magnetic.x==1&&s.attitude_valid);
    tick(&s,299);assert(triggers==0);tick(&s,301);assert(triggers==1);
    now_us=302000;irq(2,GPIO_IRQ_EDGE_RISE);now_us=304000;irq(2,GPIO_IRQ_EDGE_FALL);tick(&s,304);
    assert(s.range.status.valid&&s.range.echo_us==2000&&s.forward.status.valid);
    uint32_t forward_time=s.forward.status.sampled_ms;
    tick(&s,360);assert(triggers==1);tick(&s,367);assert(triggers==2);
    tick(&s,398);assert(!s.range.status.valid&&s.sonar_timeouts==1); /* Missing echo. */
    now_us=400000;sensor_bench_command(&s,'a',400);tick(&s,701);assert(triggers==3);
    now_us=702000;irq(2,8);now_us=703000;irq(2,4);tick(&s,703);
    tick(&s,767);assert(s.servo_command_deg==60);tick(&s,1068);assert(triggers==4);
    now_us=1069000;irq(2,8);now_us=1070000;irq(2,4);tick(&s,1070);
    assert(s.range.angle_deg==60&&s.range.status.valid&&s.forward.status.sampled_ms==703);
    assert(s.forward.status.sampled_ms!=forward_time); /* Side data cannot refresh forward timestamp. */
    sensor_bench_command(&s,'x',1070);assert(!s.servo_enabled&&servo_pwm==0);
    unsigned before=triggers;tick(&s,1200);assert(triggers==before);
    missing=true;tick(&s,1220);assert(!s.accel.status.valid&&!s.magnetic.status.valid&&s.i2c_errors>=2);
    missing=false;tick(&s,2200);tick(&s,2220);assert(s.accel_configured&&s.mag_configured&&s.accel.status.valid);
    puts("Real sensor-driver logic: GPIO allocation, motor inhibition, fresh IMU, echo IRQ/timeout, scan timing and I2C recovery passed (fake SDK).");
}
