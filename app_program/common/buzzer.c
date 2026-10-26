#include "buzzer.h"
#include "../app.h"
#include "./robot_config.h"

#define PWM_SLICE_STRIDE 0x14u

/* PWM tone at 125 MHz:
 * divider = 1000 / 16 = 62.5
 * frequency = 125 MHz / 62.5 / 1000 = 2 kHz
*/

static bool tone_init(void)
{
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

    return true;
}

static bool tone_set(bool enabled)
{
    return pwm_set_cc(BUZZER_PIN, enabled ? BUZZER_LEVEL : 0u) >= E_OK;
}

static bool initialized;
static bool active;
static bool tone_on;
static unsigned remaining;
static uint32_t deadline;

bool buzzer_init(void)
{
    initialized = false;
    active = false;
    tone_on = false;
    remaining = 0u;
    deadline = 0u;

    if (!tone_init())
    {
        return false;
    }

    initialized = true;
    return true;
}

bool buzzer_request(unsigned chirps, uint32_t now)
{
    if (!initialized || active ||
        chirps == 0u || chirps > MAX_CHIRPS)
    {
        return false;
    }

    if (!tone_set(false))
    {
        initialized = false;
        return false;
    }

    remaining = chirps;
    tone_on = false;
    deadline = now;
    active = true;
    return true;
}

bool buzzer_service(uint32_t now)
{
    if (!initialized)
    {
        return false;
    }

    if (!active || (int32_t)(now - deadline) < 0)
    {
        return true;
    }

    if (tone_on)
    {
        /* The current chirp has finished. */
        if (!tone_set(false))
        {
            initialized = false;
            return false;
        }

        tone_on = false;

        if (remaining == 0u)
        {
            active = false;
        }
        else
        {
            /* Wait through silence before the next chirp. */
            deadline = now + CHIRP_MS;
        }

        return true;
    }

    /* Begin the next chirp. */
    if (!tone_set(true))
    {
        initialized = false;
        return false;
    }

    --remaining;
    tone_on = true;
    deadline = now + CHIRP_MS;
    return true;
}