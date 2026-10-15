#include "drv_gpio_irq.h"
#define GPIO_IRQ 13u
#define GPIO_RAW (IO_BANK0_BASE + 0xf0u)
#define GPIO_ENABLE (IO_BANK0_BASE + 0x130u)
#define GPIO_STATUS (IO_BANK0_BASE + 0x150u)
static gpio_edge_handler_t handlers[30];
static bool initialized;

static void gpio_dispatch(UINT intno)
{
    (void)intno;
    uint32_t sampled_us = in_w(TIMER_TIMERAWL);
    static const unsigned order[] = {0u, 1u, 7u, 28u, 2u, 16u};
    uint32_t pending[4];
    for (unsigned bank = 0u; bank < 4u; ++bank)
    {
        pending[bank] = in_w(GPIO_STATUS + bank * 4u);
    }
    for (unsigned i = 0u; i < sizeof(order) / sizeof(order[0]); ++i)
    {
        unsigned pin = order[i], bank = pin / 8u, shift = (pin % 8u) * 4u;
        uint32_t edges = (pending[bank] >> shift) & 12u;
        if (edges != 0u && handlers[pin] != NULL)
        {
            out_w(GPIO_RAW + bank * 4u, edges << shift);
            handlers[pin](pin, edges, sampled_us);
        }
    }
}

ER drv_gpio_irq_init(void)
{
    if (tk_get_prc() != 2) { return E_CTX; }
    if (initialized) { return E_OK; }
    for (unsigned bank = 0u; bank < 4u; ++bank)
    {
        out_w(GPIO_ENABLE + bank * 4u, 0u);
    }
    T_DINT definition = {0};
    definition.intatr = TA_HLNG;
    definition.inthdr = (FP)gpio_dispatch;
    ER er = tk_def_int(GPIO_IRQ, &definition);
    if (er < E_OK) { return er; }
    ClearInt(GPIO_IRQ);
    EnableInt(GPIO_IRQ, 2u);
    initialized = true;
    return E_OK;
}

bool drv_gpio_irq_claim(unsigned pin, gpio_edge_handler_t handler)
{
    if (!initialized || pin >= 30u || handler == NULL ||
        (handlers[pin] != NULL && handlers[pin] != handler)) { return false; }
    handlers[pin] = handler;
    return true;
}

bool drv_gpio_irq_enable(unsigned pin, bool enabled)
{
    if (!initialized || pin >= 30u || handlers[pin] == NULL) { return false; }
    unsigned bank = pin / 8u, shift = (pin % 8u) * 4u;
    uint32_t mask = 12u << shift;
    UW state;
    DI(state);
    if (enabled)
    {
        out_w(GPIO_RAW + bank * 4u, mask);
        set_w(GPIO_ENABLE + bank * 4u, mask);
    }
    else { clr_w(GPIO_ENABLE + bank * 4u, mask); }
    EI(state);
    return true;
}
