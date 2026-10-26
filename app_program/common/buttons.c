#include "buttons.h"
#include "../app.h"
#include "./robot_config.h"


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

static bool configure_button_pin(unsigned pin)
{
    if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }

    set_w(GPIO(pin), GPIO_PUE);
    clr_w(GPIO(pin), GPIO_PDE);
    return true;
}

bool drv_buttons_init(void)
{
    return configure_button_pin(BUTTON_GP20) && configure_button_pin(BUTTON_GP21);
}

ButtonInputs drv_buttons_read(void)
{
    ButtonInputs inputs;

    inputs.gp20_down = gpio_get_val(BUTTON_GP20) == 0u;
    inputs.gp21_down = gpio_get_val(BUTTON_GP21) == 0u;

    return inputs;
}