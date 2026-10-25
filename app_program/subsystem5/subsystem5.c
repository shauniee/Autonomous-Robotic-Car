#include "subsystem5.h"
#include "tk/syslib.h"
#include "../common/robot_time.h"

#define ECHO_PIN 2
#define TRIGGER_PIN 3
/* Servo setup paused while learning ultrasonic sensing. */
// #define SERVO_PIN 12
// #define SERVO_CENTRE_US 1500u
// #define SERVO_SETTLE_MS 300u
//
// /* Prepare GP12 (PWM slice 6, channel A), but do not move the servo yet. */
// static ER servo_init(void)
// {
//     ER error;
//
//     /* Release PWM without resetting slices used by motors or the buzzer. */
//     clr_w(RESETS_RESET, RESETS_RESET_PWM);
//     if ((in_w(RESETS_RESET_DONE) & RESETS_RESET_PWM) == 0) {
//         return E_IO;
//     }
//
//     /* Configure only slice 6. GP13 shares this slice's frequency. */
//     out_w(PWM_CH6_BASE + PWM_CHx_CSR, 0u);
//     out_w(PWM_CH6_BASE + PWM_CHx_CTR, 0u);
//
//     /* Template clock: 125 MHz. Divide by 125 for one tick per microsecond.
//        The divider register stores the divider multiplied by 16. */
//     out_w(PWM_CH6_BASE + PWM_CHx_DIV, 125u * 16u);
//
//     /* 0..19999 gives a 20 ms frame (50 Hz). */
//     error = pwm_set_wrap(SERVO_PIN, 19999u);
//     if (error < E_OK) {
//         return error;
//     }
//
//     /* Zero duty until subsystem5_start() commands centre. */
//     error = pwm_set_cc(SERVO_PIN, 0u);
//     if (error < E_OK) {
//         return error;
//     }
//
//     return pwm_set_pin(SERVO_PIN);
// }
//
ER subsystem5_init(void)
{
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error != E_OK)
    {
        return error;
    }

    error = gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT);
    if (error != E_OK)
    {
        return error;
    }

    error = gpio_set_pin(ECHO_PIN, GPIO_MODE_IN);
    if (error < E_OK)
    {
        return error;
    }

    // return servo_init(); /* Servo setup paused. */
    return E_OK;
}
/*ULTRASONIC FUNCTIONS!!!!! :)*/
ER ultrasonic_trigger(void)
{
    ER error;

    error = gpio_set_val(TRIGGER_PIN, 0);
    if (error < E_OK)
    {
        return error;
    }
    WaitUsec(2);

    error = gpio_set_val(TRIGGER_PIN, 1);
    if (error < E_OK)
    {
        return error;
    }
    WaitUsec(10);

    return gpio_set_val(TRIGGER_PIN, 0);
}

ER ultrasonic_wait_for_echo(void)
{
    ER error;
    uint32_t start; // record the time
    uint32_t now;

    /* Record when we started waiting. */
    start = robot_time_ms();

    error = ultrasonic_trigger();
    if (error < E_OK)
    {
        return error;
    }

    /* Keep checking while Echo is LOW. */
    while (gpio_get_val(ECHO_PIN) == 0)
    {
        now = robot_time_ms();

        /* Stop waiting after 30 milliseconds. */
        if (robot_time_elapsed(now, start) >= 30u)
        {
            return E_TMOUT;
        }
    }

    /* Echo has become HIGH. */
    /* Echo is HIGH: record the start time in microseconds. */
    UW pulse_start = robot_time_us();

    /* Wait for Echo to become LOW. */
    while (gpio_get_val(ECHO_PIN) == 1)
    {
        if ((UW)(robot_time_us() - pulse_start) >= 30000u)
        {
            return E_TMOUT;
        }
    }

    /* Calculate how long Echo stayed HIGH. */
    UW pulse_end = robot_time_us();
    UW duration_us = pulse_end - pulse_start;
    /* Approximate distance in whole centimetres. */
    UW distance_cm = duration_us / 58u;

    if (duration_us >= 30000u)
    {
        return E_TMOUT;
    }

    tm_printf((UB *)"Distance: %u cm\n",
              (unsigned int)distance_cm);

    tm_printf((UB *)"Echo duration: %u us\n",
              (unsigned int)duration_us);

    return E_OK;
}

ER subsystem5_start(void)
{
    /* Servo movement paused; keep the code for a later lesson. */
    // ER error;

    // /* A 1500 us pulse requests centre, approximately 90 degrees.
    // Physically align the sensor so this position faces forward. */
    // error = pwm_set_cc(SERVO_PIN, SERVO_CENTRE_US);
    // if (error < E_OK) {
    // return error;
    // }

    // error = pwm_set_enabled(SERVO_PIN, TRUE);
    // if (error < E_OK) {
    // return error;
    // }

    // /* This delay is intentionally milliseconds; let other RTOS tasks run.
    // Initial estimate only: verify settling with the installed servo. */
    // error = tk_dly_tsk(SERVO_SETTLE_MS);
    // if (error < E_OK) {
    // return error;
    // }

    /* Non-blocking: boot never waits for an echo. Ranging is a later
       periodic job; absence of echo must not stop startup. */
    return E_OK;
}
