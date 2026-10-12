#include "buzzer.h"
#include "drv_buzzer.h"

#define CHIRP_MS 100u
#define MAX_CHIRPS 3u

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

    if (!drv_buzzer_init())
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

    if (!drv_buzzer_set_tone(false))
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
        if (!drv_buzzer_set_tone(false))
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
    if (!drv_buzzer_set_tone(true))
    {
        initialized = false;
        return false;
    }

    --remaining;
    tone_on = true;
    deadline = now + CHIRP_MS;
    return true;
}