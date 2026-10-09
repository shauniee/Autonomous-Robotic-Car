#include "drv_feedback.h"
#include "../app.h"

#define CAPTURE_PIN 20u
#define CANCEL_PIN 21u
#define BUZZER_PIN 22u
#define BUZZER_SLICE 3u
#define PWM_SLICE_STRIDE 0x14u
#define BUZZER_DIVIDER 1000u
#define BUZZER_WRAP 999u
#define BUZZER_LEVEL 500u
#define CHIRP_MS 100u
#define MAX_CHIRPS 3u

static unsigned remaining;
static bool tone_on;
static uint32_t deadline;

bool drv_feedback_init(void)
{
    if ((gpio_set_pin(CAPTURE_PIN, GPIO_MODE_IN) < E_OK) ||
        (gpio_set_pin(CANCEL_PIN, GPIO_MODE_IN) < E_OK))
    {
        return false;
    }
    set_w(GPIO(CAPTURE_PIN), GPIO_PUE);
    clr_w(GPIO(CAPTURE_PIN), GPIO_PDE);
    set_w(GPIO(CANCEL_PIN), GPIO_PUE);
    clr_w(GPIO(CANCEL_PIN), GPIO_PDE);
    if ((gpio_set_val(BUZZER_PIN, 0u) < E_OK) ||
        (gpio_set_pin(BUZZER_PIN, GPIO_MODE_OUT) < E_OK) ||
        (pwm_set_enabled(BUZZER_PIN, FALSE) < E_OK))
    {
        return false;
    }
    /* Release without resetting shared PWM; configure only S3's slice 3.
       125 MHz / 62.5 / 1000 = 2 kHz; divider is encoded in sixteenths. */
    clr_w(RESETS_RESET, RESETS_RESET_PWM);
    out_w(PWM_BASE + (BUZZER_SLICE * PWM_SLICE_STRIDE) + PWM_CHx_DIV,
        BUZZER_DIVIDER);
    remaining = 0u;
    tone_on = false;
    deadline = 0u;
    return (pwm_set_wrap(BUZZER_PIN, BUZZER_WRAP) >= E_OK) &&
        (pwm_set_cc(BUZZER_PIN, 0u) >= E_OK) &&
        (pwm_set_pin(BUZZER_PIN) >= E_OK) &&
        (pwm_set_enabled(BUZZER_PIN, TRUE) >= E_OK);
}

void drv_feedback_inputs(bool *capture_down, bool *cancel_down)
{
    *capture_down = gpio_get_val(CAPTURE_PIN) == 0u;
    *cancel_down = gpio_get_val(CANCEL_PIN) == 0u;
}

bool drv_feedback_chirps(unsigned count, uint32_t now)
{
    if (count > MAX_CHIRPS)
    {
        return false;
    }
    remaining = count;
    tone_on = false;
    deadline = now;
    return pwm_set_cc(BUZZER_PIN, 0u) >= E_OK;
}

bool drv_feedback_service(uint32_t now)
{
    if ((int32_t)(now - deadline) < 0)
    {
        return true;
    }
    if (tone_on)
    {
        if (pwm_set_cc(BUZZER_PIN, 0u) < E_OK)
        {
            return false;
        }
        tone_on = false;
        deadline = now + CHIRP_MS;
    }
    else if (remaining > 0u)
    {
        if (pwm_set_cc(BUZZER_PIN, BUZZER_LEVEL) < E_OK)
        {
            return false;
        }
        --remaining;
        tone_on = true;
        deadline = now + CHIRP_MS;
    }
    return true;
}
