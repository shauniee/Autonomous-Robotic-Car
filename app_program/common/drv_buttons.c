#include "drv_buttons.h"
#include "../app.h"
#include "./robot_config.h"


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