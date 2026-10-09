#include "gpio_capture.h"
#include <tk/syslib.h>

#define GPIO_BANK_IRQ 13u
#define GPIO_IRQ_PRIORITY 5
#define GPIO_RAW_IRQ 0x400140f0u
#define GPIO_PROC0_ENABLE 0x40014100u
#define GPIO_PROC0_STATUS 0x40014120u
#define GPIO_PROC1_ENABLE 0x40014130u
#define GPIO_PROC1_STATUS 0x40014150u
#define GPIO_LEVELS 0xd0000004u
#define TIMER_LOW_US 0x40054028u
#define CAPTURE_CLIENTS 2u

static struct
{
    uint32_t mask;
    GpioCaptureCallback callback;
} clients[CAPTURE_CLIENTS];
static unsigned client_count;
static bool initialized;

uint32_t gpio_capture_time_us(void)
{
    return in_w(TIMER_LOW_US);
}

static void capture_isr(UINT intno)
{
    (void)intno;
    uint32_t status_base = GPIO_PROC0_STATUS;
#if TK_SUPPORT_SMP
    status_base = GPIO_PROC1_STATUS;
#endif
    uint32_t changed = 0u;
    uint32_t ambiguous = 0u;
    uint32_t timestamp = gpio_capture_time_us();
    uint32_t levels = in_w(GPIO_LEVELS);
    for (unsigned group = 0u; group < 4u; ++group)
    {
        uint32_t status = in_w(status_base + group * 4u);
        /* Edge bits are W1C. Never acknowledge level interrupts. */
        out_w(GPIO_RAW_IRQ + group * 4u, status & 0xccccccccu);
        for (unsigned slot = 0u; slot < 8u; ++slot)
        {
            uint32_t edges = (status >> (slot * 4u)) & 12u;
            uint32_t bit = 1u << (group * 8u + slot);
            if (edges != 0u)
            {
                changed |= bit;
            }
            if (edges == 12u)
            {
                ambiguous |= bit;
            }
        }
    }
    /* Registration order is encoder first, barcode second: barcode sees the
       same bounded dispatcher observation's updated encoder context. */
    for (unsigned i = 0u; i < client_count; ++i)
    {
        if ((changed & clients[i].mask) != 0u)
        {
            clients[i].callback(levels, changed & clients[i].mask,
                ambiguous & clients[i].mask, timestamp);
        }
    }
}

ER gpio_capture_init(void)
{
    if (initialized)
    {
        return E_OBJ;
    }
    T_DINT interrupt = {0};
    interrupt.intatr = TA_HLNG;
    interrupt.inthdr = (FP)capture_isr;
    ER error = tk_def_int(GPIO_BANK_IRQ, &interrupt);
    initialized = error >= E_OK;
    return error;
}

ER gpio_capture_register(uint32_t mask, GpioCaptureCallback callback)
{
    const uint32_t permitted = (1u << 0) | (1u << 1) | (1u << 7) |
        (1u << 16) | (1u << 28);
    if (!initialized || (callback == NULL) || (mask == 0u) ||
        ((mask & ~permitted) != 0u) || (client_count == CAPTURE_CLIENTS))
    {
        return E_PAR;
    }
    for (unsigned i = 0u; i < client_count; ++i)
    {
        if ((clients[i].mask & mask) != 0u)
        {
            return E_OBJ;
        }
    }
    clients[client_count].mask = mask;
    clients[client_count].callback = callback;
    ++client_count;
    return E_OK;
}

ER gpio_capture_enable(void)
{
    if (!initialized)
    {
        return E_OBJ;
    }
#if TK_SUPPORT_SMP
    if (tk_get_prc() != TP_PRC2)
    {
        return E_CTX;
    }
#endif
    uint32_t enable_base = GPIO_PROC0_ENABLE;
#if TK_SUPPORT_SMP
    enable_base = GPIO_PROC1_ENABLE;
#endif
    uint32_t mask = 0u;
    for (unsigned i = 0u; i < client_count; ++i)
    {
        mask |= clients[i].mask;
    }
    for (unsigned group = 0u; group < 4u; ++group)
    {
        uint32_t edges = 0u;
        for (unsigned slot = 0u; slot < 8u; ++slot)
        {
            if ((mask & (1u << (group * 8u + slot))) != 0u)
            {
                edges |= 12u << (slot * 4u);
            }
        }
        out_w(GPIO_RAW_IRQ + group * 4u, edges);
        set_w(enable_base + group * 4u, edges);
    }
    EnableInt(GPIO_BANK_IRQ, GPIO_IRQ_PRIORITY);
    return E_OK;
}
