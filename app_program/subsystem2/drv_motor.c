#include "subsystem2.h"
#include "drv_motor.h"
/* Template clk_sys is 125 MHz. PWM DIV is 8.4 fixed point:
   125 MHz / 6.25 / 1000 = 20 kHz, carrier maximum. */
#define MOTOR_WRAP 999u
#define MOTOR_DIV_8_4 100u
/* Verify motor lead polarity on the actual robot, wheels raised. */
#define LEFT_FORWARD_A 1
#define RIGHT_FORWARD_A 1
void drv_motor_brake(void) {
    for (unsigned p=8; p<=11; ++p) pwm_set_cc(p,0);
}
void drv_motor_init(void) {
    /* LOW/LOW is the carrier's brake state; establish before PWM mux. */
    for (unsigned p=8; p<=11; ++p) {
        gpio_set_val(p,0); gpio_set_pin(p,GPIO_MODE_OUT);
    }
    /* USE_PTMR=0: the template does not release PWM at boot. Release only;
       never reset the shared block after another subsystem owns a slice. */
    clr_w(RESETS_RESET,RESETS_RESET_PWM);
    for (unsigned p=8; p<=10; p+=2) {
        pwm_set_enabled(p,FALSE);
        out_w(PWM_BASE + (p/2)*0x14 + PWM_CHx_DIV, MOTOR_DIV_8_4);
        out_w(PWM_BASE + (p/2)*0x14 + PWM_CHx_CTR, 0);
        pwm_set_wrap(p,MOTOR_WRAP);
        pwm_set_cc(p,0); pwm_set_cc(p+1,0);
        pwm_set_pin(p); pwm_set_pin(p+1);
        pwm_set_enabled(p,TRUE);
    }
}
static void wheel(unsigned p, float duty, int forward_a) {
    UW cc=(UW)(duty*(MOTOR_WRAP+1));
    pwm_set_cc(p,forward_a ? cc : 0);
    pwm_set_cc(p+1,forward_a ? 0 : cc);
}
void drv_motor_apply(float left, float right) {
    wheel(8,left,LEFT_FORWARD_A); wheel(10,right,RIGHT_FORWARD_A);
}
