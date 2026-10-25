/* Use the port guard when mixing ISO C headers with tk/syslib.h. */
#define PROHIBIT_DEF_SIZE_T
#include <math.h>
#include "drv_motor.h"
#include "../common/robot_config.h"
/* Template clk_sys is 125 MHz. PWM DIV is 8.4 fixed point:
   125 MHz / 6.25 / 1000 = 20 kHz, carrier maximum. */
/* Verify motor lead polarity on the actual robot, wheels raised. */

static ER motor_gpio_low(void)
{
    ER result = E_OK;
    for (unsigned pin = 8u; pin <= 11u; ++pin)
    {
        ER er = gpio_set_val(pin, 0u);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
        er = gpio_set_pin(pin, GPIO_MODE_OUT);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
    }
    return result;
}

ER drv_motor_brake(void)
{
    ER result = E_OK;
    /* Attempt every channel even if an earlier write fails. */
    for (unsigned pin = 8u; pin <= 11u; ++pin)
    {
        ER er = pwm_set_cc(pin, 0u);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
    }
    if (result < E_OK)
    {
        /* Best effort SIO fallback; retain the first PWM error. */
        (void)motor_gpio_low();
    }
    return result;
}

static ER motor_setup_failed(ER error)
{
    (void)drv_motor_brake();
    /* Partial setup may leave some pins outside the PWM function. */
    (void)motor_gpio_low();
    return error;
}

static ER motor_setup_slice(unsigned pin)
{
    ER er = pwm_set_enabled(pin, FALSE);
    if (er >= E_OK)
    {
        out_w(PWM_BASE + (pin / 2u) * 0x14u + PWM_CHx_DIV,
              MOTOR_DIV_8_4);
        out_w(PWM_BASE + (pin / 2u) * 0x14u + PWM_CHx_CTR, 0u);
        er = pwm_set_wrap(pin, MOTOR_WRAP);
    }
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin, 0u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin + 1u, 0u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_pin(pin);
    }
    if (er >= E_OK)
    {
        er = pwm_set_pin(pin + 1u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_enabled(pin, TRUE);
    }
    return er;
}

ER drv_motor_init(void)
{
    /* LOW/LOW is the carrier brake state; establish it before PWM mux. */
    ER er = motor_gpio_low();
    if (er >= E_OK)
    {
        /* Release only; never reset the shared block or another slice. */
        clr_w(RESETS_RESET, RESETS_RESET_PWM);
        for (unsigned pin = 8u; (pin <= 10u) && (er >= E_OK); pin += 2u)
        {
            er = motor_setup_slice(pin);
        }
    }
    if (er < E_OK)
    {
        er = motor_setup_failed(er);
    }
    return er;
}

/* Signed duty: positive drives the wheel forward, negative backward.
   forward_a is the per-motor polarity constant (robot_config.h): the channel
   that moves the wheel forward. Magnitude is clamped to MOTION_MAX_DUTY. */
static ER motor_wheel(unsigned pin, float duty, int forward_a)
{
    float magnitude = duty < 0.0f ? -duty : duty;
    if (magnitude > MOTION_MAX_DUTY)
    {
        magnitude = MOTION_MAX_DUTY;
    }
    /* Clamped magnitude <= MOTION_MAX_DUTY keeps compare within MOTOR_WRAP. */
    UW compare = (UW)(magnitude * (MOTOR_WRAP + 1u));
    int a_active = (duty >= 0.0f) ? (forward_a != 0) : (forward_a == 0);
    ER er = pwm_set_cc(pin, a_active ? compare : 0u);
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin + 1u, a_active ? 0u : compare);
    }
    return er;
}

ER drv_motor_apply(float left, float right)
{
    ER er = E_PAR;
    /* Reject non-finite values before writing either wheel. */
    if (isfinite(left) && isfinite(right))
    {
        er = motor_wheel(8u, left, LEFT_FORWARD_A);
        if (er >= E_OK)
        {
            er = motor_wheel(10u, right, RIGHT_FORWARD_A);
        }
    }
    if (er < E_OK)
    {
        (void)drv_motor_brake();
    }
    return er;
}
/* End of drv_motor.c */
