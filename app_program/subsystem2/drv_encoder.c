#include "drv_encoder.h"
#include "../common/drv_gpio_irq.h"
#include "../common/robot_config.h"
static volatile uint32_t counts[2], faults, epoch = 1u;
static unsigned previous[2];
static bool ready;
static const unsigned pins[2][2] = {{28u, 7u}, {1u, 0u}};
int drv_encoder_transition(unsigned before, unsigned after)
{
    static const int8_t table[16] = {0, 1, -1, 2,  -1, 0,  2, 1,
                                     1, 2, 0,  -1, 2,  -1, 1, 0};
    return before < 4u && after < 4u ? table[before * 4u + after] : 2;
}
static unsigned level(unsigned wheel)
{
    return (unsigned)gpio_get_val(pins[wheel][0]) * 2u +
           (unsigned)gpio_get_val(pins[wheel][1]);
}
static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)sampled_us;
    unsigned wheel = pin == 28u || pin == 7u ? 0u : 1u;
    unsigned current = level(wheel);
    int delta = drv_encoder_transition(previous[wheel], current);
    if (edges == 12u || delta == 2)
    {
        ++faults;
    }
    else
    {
        int sign = wheel == 0u ? robot_config.left_encoder_sign
                               : robot_config.right_encoder_sign;
        counts[wheel] += (uint32_t)(delta * sign);
    }
    previous[wheel] = current;
}
bool drv_encoder_init(void)
{
    if (drv_gpio_irq_init() < E_OK)
    {
        return false;
    }
    for (unsigned wheel = 0u; wheel < 2u; ++wheel)
    {
        for (unsigned channel = 0u; channel < 2u; ++channel)
        {
            unsigned pin = pins[wheel][channel];
            if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK ||
                !drv_gpio_irq_claim(pin, edge))
            {
                return false;
            }
        }
        previous[wheel] = level(wheel);
    }
    ready = true;
    for (unsigned wheel = 0u; wheel < 2u; ++wheel)
    {
        for (unsigned channel = 0u; channel < 2u; ++channel)
        {
            if (!drv_gpio_irq_enable(pins[wheel][channel], true))
            {
                ready = false;
                return false;
            }
        }
    }
    return true;
}
bool drv_encoder_copy(EncoderCapture *out)
{
    if (out == NULL || !ready)
    {
        return false;
    }
    UW state;
    DI(state);
    *out = (EncoderCapture){counts[0], counts[1], in_w(TIMER_TIMERAWL),
                            epoch,     faults,    faults == 0u};
    EI(state);
    return out->valid;
}

bool drv_encoder_rearm(void)
{
    if (!ready || tk_get_prc() != 2)
    {
        return false;
    }
    UW state;
    DI(state);
    for (unsigned w = 0u; w < 2u; ++w)
    {
        for (unsigned c = 0u; c < 2u; ++c)
        {
            (void)drv_gpio_irq_enable(pins[w][c], false);
        }
        previous[w] = level(w);
        counts[w] = 0u;
    }
    faults = 0u;
    ++epoch;
    for (unsigned w = 0u; w < 2u; ++w)
    {
        for (unsigned c = 0u; c < 2u; ++c)
        {
            (void)drv_gpio_irq_enable(pins[w][c], true);
        }
    }
    EI(state);
    return true;
}
