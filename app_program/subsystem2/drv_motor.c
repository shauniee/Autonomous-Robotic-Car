#include "drv_motor.h"
#include "subsystem2.h"

#define MOTOR_WRAP 999u
#define MOTOR_DIV_8_4 100u
#define MOTOR_LEFT_PIN 8u
#define MOTOR_RIGHT_PIN 10u
#define MOTOR_LAST_PIN 11u
#define PWM_SLICE_STRIDE 0x14u
#define PWM_FUNCTION_MASK 0x1fu
#define RESET_POLL_LIMIT 4096u

/* Existing bench polarity; confirm against the installed motor leads.
   125 MHz / 6.25 / 1000 = 20 kHz; only slices 4 and 5 are owned here. */
#define LEFT_FORWARD_A true
#define RIGHT_FORWARD_A true

static bool initialized;

static UW slice_register(unsigned pin, unsigned offset)
{
    return PWM_BASE + ((pin / 2u) * PWM_SLICE_STRIDE) + offset;
}

static void fallback_low(void)
{
    /* Best effort if setup/readback fails; do not reset shared PWM. */
    for (unsigned pin = MOTOR_LEFT_PIN; pin <= MOTOR_LAST_PIN; ++pin)
    {
        (void)gpio_set_val(pin, 0u);
        (void)gpio_set_pin(pin, GPIO_MODE_OUT);
    }
}

bool drv_motor_brake(void)
{
    bool success = true;
    for (unsigned pin = MOTOR_LEFT_PIN; pin <= MOTOR_LAST_PIN; ++pin)
    {
        if (pwm_set_cc(pin, 0u) < E_OK)
        {
            success = false;
        }
    }
    success = success &&
        (in_w(slice_register(MOTOR_LEFT_PIN, PWM_CHx_CC)) == 0u) &&
        (in_w(slice_register(MOTOR_RIGHT_PIN, PWM_CHx_CC)) == 0u);
    float left;
    float right;
    success = success && initialized &&
        drv_motor_output_copy(&left, &right) &&
        (left == 0.0f) && (right == 0.0f);
    if (!success)
    {
        initialized = false;
        fallback_low();
    }
    return success;
}

static bool configure_slice(unsigned pin)
{
    if (pwm_set_enabled(pin, FALSE) < E_OK)
    {
        return false;
    }
    out_w(slice_register(pin, PWM_CHx_DIV), MOTOR_DIV_8_4);
    out_w(slice_register(pin, PWM_CHx_CTR), 0u);
    if ((pwm_set_wrap(pin, MOTOR_WRAP) < E_OK) ||
        (pwm_set_cc(pin, 0u) < E_OK) ||
        (pwm_set_cc(pin + 1u, 0u) < E_OK) ||
        (pwm_set_pin(pin) < E_OK) ||
        (pwm_set_pin(pin + 1u) < E_OK) ||
        (pwm_set_enabled(pin, TRUE) < E_OK))
    {
        return false;
    }
    return (in_w(slice_register(pin, PWM_CHx_DIV)) == MOTOR_DIV_8_4) &&
        (in_w(slice_register(pin, PWM_CHx_TOP)) == MOTOR_WRAP) &&
        (in_w(slice_register(pin, PWM_CHx_CC)) == 0u) &&
        ((in_w(slice_register(pin, PWM_CHx_CSR)) & PWM_CH_CSR_EN) != 0u) &&
        ((in_w(GPIO_CTRL(pin)) & PWM_FUNCTION_MASK) ==
            GPIO_CTRL_FUNCSEL_PWM) &&
        ((in_w(GPIO_CTRL(pin + 1u)) & PWM_FUNCTION_MASK) ==
            GPIO_CTRL_FUNCSEL_PWM);
}

bool drv_motor_init(void)
{
    initialized = false;
    for (unsigned pin = MOTOR_LEFT_PIN; pin <= MOTOR_LAST_PIN; ++pin)
    {
        if ((gpio_set_val(pin, 0u) < E_OK) ||
            (gpio_set_pin(pin, GPIO_MODE_OUT) < E_OK))
        {
            fallback_low();
            return false;
        }
    }
    /* Release only. Another subsystem already owns buzzer slice 3. */
    clr_w(RESETS_RESET, RESETS_RESET_PWM);
    bool released = false;
    for (unsigned poll = 0u; poll < RESET_POLL_LIMIT; ++poll)
    {
        if ((in_w(RESETS_RESET_DONE) & RESETS_RESET_PWM) != 0u)
        {
            released = true;
            break;
        }
    }
    if (!released || !configure_slice(MOTOR_LEFT_PIN) ||
        !configure_slice(MOTOR_RIGHT_PIN))
    {
        fallback_low();
        return false;
    }
    initialized = true;
    return drv_motor_brake();
}

static bool wheel(unsigned pin, float duty, bool forward_a)
{
    UW level = (UW)(duty * (float)(MOTOR_WRAP + 1u));
    UW expected = forward_a ? level : (level << 16);
    unsigned inactive = forward_a ? pin + 1u : pin;
    unsigned active = forward_a ? pin : pin + 1u;
    return (pwm_set_cc(inactive, 0u) >= E_OK) &&
        (pwm_set_cc(active, level) >= E_OK) &&
        (in_w(slice_register(pin, PWM_CHx_CC)) == expected);
}

bool drv_motor_apply(float left, float right)
{
    if (!initialized || !motion_duties_valid(left, right))
    {
        (void)drv_motor_brake();
        return false;
    }
    if (!wheel(MOTOR_LEFT_PIN, left, LEFT_FORWARD_A) ||
        !wheel(MOTOR_RIGHT_PIN, right, RIGHT_FORWARD_A))
    {
        initialized = false;
        (void)drv_motor_brake();
        fallback_low();
        return false;
    }
    return true;
}

static bool wheel_output(unsigned pin, bool forward_a, float *duty)
{
    UW value = in_w(slice_register(pin, PWM_CHx_CC));
    UW active = forward_a ? (value & 0xffffu) : (value >> 16);
    UW inactive = forward_a ? (value >> 16) : (value & 0xffffu);
    if ((inactive != 0u) ||
        (active > (UW)(MOTION_MAX_DUTY * (float)(MOTOR_WRAP + 1u))) ||
        ((in_w(slice_register(pin, PWM_CHx_CSR)) & PWM_CH_CSR_EN) == 0u) ||
        (in_w(slice_register(pin, PWM_CHx_TOP)) != MOTOR_WRAP) ||
        (in_w(slice_register(pin, PWM_CHx_DIV)) != MOTOR_DIV_8_4) ||
        ((in_w(GPIO_CTRL(pin)) & PWM_FUNCTION_MASK) !=
            GPIO_CTRL_FUNCSEL_PWM) ||
        ((in_w(GPIO_CTRL(pin + 1u)) & PWM_FUNCTION_MASK) !=
            GPIO_CTRL_FUNCSEL_PWM))
    {
        return false;
    }
    *duty = (float)active / (float)(MOTOR_WRAP + 1u);
    return true;
}

bool drv_motor_output_copy(float *left, float *right)
{
    if (!initialized || (left == NULL) || (right == NULL))
    {
        return false;
    }
    float left_output;
    float right_output;
    if (!wheel_output(MOTOR_LEFT_PIN, LEFT_FORWARD_A, &left_output) ||
        !wheel_output(MOTOR_RIGHT_PIN, RIGHT_FORWARD_A, &right_output))
    {
        return false;
    }
    *left = left_output;
    *right = right_output;
    return true;
}
