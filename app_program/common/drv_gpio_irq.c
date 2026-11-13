#include "drv_gpio_irq.h"
#include "robot_time.h"

#define GPIO_IRQ_NUMBER 13u /* IO_IRQ_BANK0 */
#define GPIO_INTR(bank)     (IO_BANK0_BASE + 0xf0u + (bank) * 4u)
#define GPIO_PROC_ENABLE(bank)  (IO_BANK0_BASE + 0x130u + (bank) * 4u)
#define GPIO_PROC_STATUS(bank)  (IO_BANK0_BASE + 0x150u + (bank) * 4u)
#define GPIO_BANKS ((GPIO_IRQ_PIN_COUNT + 7u) / 8u)
#define GPIO_EDGE_MASK (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

static gpio_edge_handler_t handlers[GPIO_IRQ_PIN_COUNT];
static bool initialized;

/* Interrupt handler on core 1. Only edge bits of claimed pins are read and
   acknowledged, so unclaimed pins never block the dispatch. */
static void gpio_dispatch(UINT intno)
{
    (void)intno;
    uint32_t sampled_us = robot_time_us();
    uint32_t pending[GPIO_BANKS];
    for (unsigned bank = 0u; bank < GPIO_BANKS; ++bank)
    {
        pending[bank] = in_w(GPIO_PROC_STATUS(bank));
    }
    for (unsigned pin = 0u; pin < GPIO_IRQ_PIN_COUNT; ++pin)
    {
        gpio_edge_handler_t handler = handlers[pin];
        if (NULL != handler)
        {
            unsigned bank = pin / 8u, shift = (pin % 8u) * 4u;
            uint32_t edges = (pending[bank] >> shift) & GPIO_EDGE_MASK;
            if (edges != 0u)
            {
                out_w(GPIO_INTR(bank), edges << shift);
                handler(pin, edges, sampled_us);
            }
        }
    }
}

ER drv_gpio_irq_init(void)
{
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
    if (initialized)
    {
        return E_OK;
    }
    for (unsigned bank = 0u; bank < GPIO_BANKS; ++bank)
    {
        out_w(GPIO_PROC_ENABLE(bank), 0u);
    }
    T_DINT definition = {0};
    definition.intatr = TA_HLNG;
    definition.inthdr = (FP)gpio_dispatch;
    ER er = tk_def_int(GPIO_IRQ_NUMBER, &definition);
    if (er < E_OK)
    {
        return er;
    }
    ClearInt(GPIO_IRQ_NUMBER);
    EnableInt(GPIO_IRQ_NUMBER, 2u);
    initialized = true;
    return E_OK;
}

bool drv_gpio_irq_claim(unsigned pin, gpio_edge_handler_t handler)
{
    if (!initialized || pin >= GPIO_IRQ_PIN_COUNT || NULL == handler ||
        (handlers[pin] != NULL && handlers[pin] != handler))
    {
        return false;
    }
    handlers[pin] = handler;
    return true;
}

bool drv_gpio_irq_enable(unsigned pin, bool enabled)
{
    if (!initialized || pin >= GPIO_IRQ_PIN_COUNT || NULL == handlers[pin])
    {
        return false;
    }
    unsigned bank = pin / 8u, shift = (pin % 8u) * 4u;
    uint32_t mask = GPIO_EDGE_MASK << shift;
    UW state;
    DI(state);
    if (enabled)
    {
        out_w(GPIO_INTR(bank), mask);
        set_w(GPIO_PROC_ENABLE(bank), mask);
    }
    else
    {
        clr_w(GPIO_PROC_ENABLE(bank), mask);
    }
    EI(state);
    return true;
}
