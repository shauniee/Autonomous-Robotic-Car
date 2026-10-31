#include "drv_ir_tracking.h"
#include "../app.h"
#include "../common/robot_config.h"

/* Connect each module's DOUT, with AOUT disconnected. */
static bool configure_input(unsigned pin)
{
    if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK)
    {
        return false;
    }
    clr_w(GPIO(pin), GPIO_PUE | GPIO_PDE);
    return true;
}

bool drv_ir_tracking_init(void)
{
    return configure_input(IR_LEFT_PIN) && configure_input(IR_RIGHT_PIN);
}

bool drv_ir_tracking_read(IrRaw *raw)
{
    if (raw == NULL)
    {
        return false;
    }
    /* One GPIO snapshot keeps both sensor states from the same instant. */
    uint32_t levels = in_w(GPIO_IN);
    raw->left = (uint16_t)((levels >> IR_LEFT_PIN) & 1u);
    raw->right = (uint16_t)((levels >> IR_RIGHT_PIN) & 1u);
    return true;
}
