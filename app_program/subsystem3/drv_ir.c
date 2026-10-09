#include "drv_ir.h"
#include "../app.h"

#define ADC_REG_BASE 0x4004c000u
#define ADC_CONTROL ADC_REG_BASE
#define ADC_RESULT (ADC_REG_BASE + 4u)
#define ADC_ENABLE 1u
#define ADC_START_ONCE (1u << 2)
#define ADC_READY (1u << 8)
#define ADC_ERROR (1u << 9)
#define ADC_CHANNEL_SHIFT 12u
#define ADC_BUDGET_US 2000u
#define ADC_POLL_LIMIT 4096u
#define ADC_AVERAGE_COUNT 8u
#define IR_LEFT_PIN 26u
#define IR_RIGHT_PIN 27u
#define GPIO_NULL_FUNCTION 31u

/* Single S3 task owns the ADC and this whole-pair poll budget. */
static unsigned polls;

static uint32_t clock_us(void)
{
    return in_w(TIMER_TIMERAWL);
}

static bool budget_available(uint32_t started)
{
    return (polls < ADC_POLL_LIMIT) &&
        ((uint32_t)(clock_us() - started) < ADC_BUDGET_US);
}

static bool ready(uint32_t started)
{
    while ((in_w(ADC_CONTROL) & ADC_READY) == 0u)
    {
        ++polls;
        if (!budget_available(started))
        {
            return false;
        }
    }
    return budget_available(started);
}

bool drv_ir_init(void)
{
    polls = 0u;
    uint32_t started = clock_us();
    clr_w(RESETS_RESET, RESETS_RESET_ADC);
    while ((in_w(RESETS_RESET_DONE) & RESETS_RESET_ADC) == 0u)
    {
        ++polls;
        if (!budget_available(started))
        {
            return false;
        }
    }
    for (unsigned pin = IR_LEFT_PIN; pin <= IR_RIGHT_PIN; ++pin)
    {
        out_w(GPIO_CTRL(pin), GPIO_NULL_FUNCTION);
        clr_w(GPIO(pin), GPIO_IE | GPIO_PUE | GPIO_PDE);
        /* Disable digital output on analogue pads. */
        set_w(GPIO(pin), GPIO_OD);
    }
    out_w(ADC_CONTROL, ADC_ENABLE);
    return ready(started);
}

static bool channel(unsigned mux, uint16_t *value, uint32_t started)
{
    uint32_t control = ADC_ENABLE | (mux << ADC_CHANNEL_SHIFT) | ADC_ERROR;
    out_w(ADC_CONTROL, control);
    uint32_t sum = 0u;
    for (unsigned conversion = 0u; conversion <= ADC_AVERAGE_COUNT;
         ++conversion)
    {
        out_w(ADC_CONTROL, control | ADC_START_ONCE);
        if (!ready(started) || ((in_w(ADC_CONTROL) & ADC_ERROR) != 0u))
        {
            return false;
        }
        uint32_t raw = in_w(ADC_RESULT) & IR_ADC_MAX;
        if (conversion > 0u)
        {
            sum += raw; /* Discard first conversion after each mux change. */
        }
    }
    /* Eight 12-bit readings fit uint32_t; their average fits uint16_t. */
    *value = (uint16_t)(sum / ADC_AVERAGE_COUNT);
    return true;
}

bool drv_ir_read(IrRaw *raw)
{
    if (raw == NULL)
    {
        return false;
    }
    polls = 0u;
    IrRaw pair = {0};
    uint32_t started = clock_us();
    if (!channel(0u, &pair.left, started) ||
        !channel(1u, &pair.right, started) || !budget_available(started))
    {
        return false;
    }
    /* Never expose a partial pair, even if the right channel fails. */
    *raw = pair;
    return true;
}
