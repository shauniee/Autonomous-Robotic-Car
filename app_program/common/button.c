#include "button.h"
#include <stddef.h>

#define BUTTON_DEBOUNCE_MS 30u

void button_init(Button *button, uint32_t now)
{
    if (button != NULL)
    {
        *button = (Button){.raw_changed_ms = now};
    }
}

void button_disarm(Button *button)
{
    if (button != NULL)
    {
        button->press_allowed = false;
    }
}

ButtonEvent button_update(Button *button, bool down, uint32_t now)
{
    if (button == NULL)
    {
        return BUTTON_EVENT_NONE;
    }

    if (down != button->last_raw_down)
    {
        button->last_raw_down = down;
        button->raw_changed_ms = now;
    }

    if ((uint32_t)(now - button->raw_changed_ms) < BUTTON_DEBOUNCE_MS)
    {
        return BUTTON_EVENT_NONE;
    }

    if (!button->last_raw_down)
    {
        button->press_allowed = true;
    }

    if (button->debounced_down == button->last_raw_down)
    {
        return BUTTON_EVENT_NONE;
    }

    button->debounced_down = button->last_raw_down;

    if (!button->debounced_down)
    {
        return BUTTON_EVENT_RELEASED;
    }

    if (!button->press_allowed)
    {
        return BUTTON_EVENT_IGNORED_PRESS;
    }

    button->press_allowed = false;
    return BUTTON_EVENT_PRESSED;
}