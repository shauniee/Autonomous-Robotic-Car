#include "../app.h"
#include "drv_feedback.h"
static unsigned remaining;
static bool tone_on;
static uint32_t deadline;
void drv_feedback_init(void) {
    gpio_set_pin(20,GPIO_MODE_IN); gpio_set_pin(21,GPIO_MODE_IN);
    set_w(GPIO(20),GPIO_PUE); clr_w(GPIO(20),GPIO_PDE);
    set_w(GPIO(21),GPIO_PUE); clr_w(GPIO(21),GPIO_PDE);
    gpio_set_val(22,0); gpio_set_pin(22,GPIO_MODE_OUT);
    pwm_set_enabled(22,FALSE);
    out_w(PWM_BASE+3*0x14+PWM_CHx_DIV,1000); /* divider 62.5 */
    pwm_set_wrap(22,999); pwm_set_cc(22,0); pwm_set_pin(22);
    pwm_set_enabled(22,TRUE); /* 2 kHz; PWM3 independent of motor PWM4/5. */
}
void drv_feedback_chirps(unsigned count, uint32_t now) {
    remaining=count; tone_on=false; deadline=now;
    pwm_set_cc(22,0);
}
void drv_feedback_service(uint32_t now) {
    if ((int32_t)(now-deadline)<0) return;
    if (tone_on) {
        pwm_set_cc(22,0); tone_on=false; deadline=now+100;
    } else if (remaining) {
        --remaining; pwm_set_cc(22,500); tone_on=true; deadline=now+100;
    }
}
/* Hardware TIMER and watchdog share a 1 us tick from the 12 MHz reference.
   The kernel's scheduling tick is SysTick. Do not reset TIMER: WiFi reads it. */
void drv_timebase_init(void) {
    out_w(0x4005802c,(1u<<9)|12u);
    clr_w(RESETS_RESET,RESETS_RESET_TIMER);
}
/* RP2040 watchdog registers, aligned with the SDK/datasheet sequence. */
void drv_watchdog_enable(void) {
    clr_w(0x40058000,1u<<30);
    set_w(0x40010008,0x1ffffu & ~3u); /* Reset all except crystal/ROSC. */
    out_w(0x4005801c,0); /* No reboot-to-address bootrom scratch magic. */
    out_w(0x40058004,500000); /* RP2040 load factor 2: 250 ms. */
    set_w(0x40058000,1u<<30);
}
void drv_watchdog_feed(void) { out_w(0x40058004,500000); }
