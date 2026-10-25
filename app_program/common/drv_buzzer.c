#include "drv_buzzer.h"
#include "../app.h"
#include "./robot_config.h"

#define PWM_SLICE_STRIDE 0x14u

/* 
 * At 125 MHz:
 * divider = 1000 / 16 = 62.5
 * frequency = 125 MHz / 62.5 / 1000 = 2 kHz
*/

static bool initialized;

bool drv_buzzer_init(void)
{
    initialized = false;

    /* Establish LOW before connecting PWM to the pin. */
    if ((gpio_set_val(BUZZER_PIN, 0u) < E_OK) ||
        (gpio_set_pin(BUZZER_PIN, GPIO_MODE_OUT) < E_OK) ||
        (pwm_set_enabled(BUZZER_PIN, FALSE) < E_OK))
    {
        return false;
    }

    /* Release PWM without resetting other owners' slices. */
    clr_w(RESETS_RESET, RESETS_RESET_PWM);

    out_w(
        PWM_BASE + BUZZER_SLICE * PWM_SLICE_STRIDE + PWM_CHx_DIV,
        BUZZER_DIVIDER);

    if ((pwm_set_wrap(BUZZER_PIN, BUZZER_WRAP) < E_OK) ||
        (pwm_set_cc(BUZZER_PIN, 0u) < E_OK) ||
        (pwm_set_pin(BUZZER_PIN) < E_OK) ||
        (pwm_set_enabled(BUZZER_PIN, TRUE) < E_OK))
    {
        return false;
    }

    initialized = true;
    return true;
}

bool drv_buzzer_set_tone(bool enabled)
{
    if (!initialized)
    {
        return false;
    }

    return pwm_set_cc(BUZZER_PIN, enabled ? BUZZER_LEVEL : 0u) >= E_OK;
}