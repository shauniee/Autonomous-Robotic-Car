#include "drv_ir_tracking.h"
#include "../app.h"
#include "../common/robot_time.h"

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
#define IR_LEFT_ADC_CHANNEL 0u
#define IR_RIGHT_ADC_CHANNEL 1u
#define GPIO_NULL_FUNCTION 31u

/* IR owns the ADC, no two tasks will not amend the same counter while reading the ADC */
static unsigned polls;

static bool pool_within_limit(void)
{
    return (polls < ADC_POLL_LIMIT);
}

static bool time_within_limit(uint32_t started)
{
    return ((uint32_t)(robot_time_us() - started) < ADC_BUDGET_US);
}

static bool budget_available(uint32_t started)
{
    return pool_within_limit() && time_within_limit(started);
}

static bool wait_for_bit(uint32_t register_address, uint32_t bit_mask, uint32_t started)
{
    while ((in_w(register_address) & bit_mask) == 0u)
    {
        ++polls;
        if (!budget_available(started))
        {
            return false;
        }
    }
    return budget_available(started);
}

static bool wait_for_adc_ready(uint32_t started)
{
    return wait_for_bit(ADC_CONTROL, ADC_READY, started);
}

static bool wait_for_adc_reset(uint32_t started)
{
    return wait_for_bit(RESETS_RESET_DONE, RESETS_RESET_ADC, started);
}

static void configure_adc_pin(unsigned pin)
{
    /* Disconnect digital peripheral functions. */
    out_w(GPIO_CTRL(pin), GPIO_NULL_FUNCTION);

    /* Disable digital input and internal pull resistors. */
    clr_w(GPIO(pin), GPIO_IE | GPIO_PUE | GPIO_PDE);

    /* Prevent digital output from driving the pad. */
    set_w(GPIO(pin), GPIO_OD);
}

/* Select an input and return the control bits needed for conversions */
static uint32_t adc_select_channel(unsigned mux)
{
    uint32_t control = ADC_ENABLE | (mux << ADC_CHANNEL_SHIFT) | ADC_ERROR;
    out_w(ADC_CONTROL, control);
    return control;
}

/* Perform one checked conversion, settling precaution after switching inputs */
static bool adc_read_once(uint32_t control, uint32_t *value, uint32_t started)
{
    out_w(ADC_CONTROL, control | ADC_START_ONCE);
    if (!wait_for_adc_ready(started) || ((in_w(ADC_CONTROL) & ADC_ERROR) != 0u))
    {
        return false;
    }
    uint32_t raw = in_w(ADC_RESULT) & IR_ADC_MAX;
    *value = (uint32_t)raw;
    return true;
}

static bool channel(unsigned mux, uint16_t *value, uint32_t started)
{
    uint32_t control = adc_select_channel(mux);
    uint32_t sample;

    if (!adc_read_once(control, &sample, started))
    {
        return false;
    }
    uint32_t sum = 0u;

    for (unsigned i = 0u; i < ADC_AVERAGE_COUNT; ++i)
    {
        if (!adc_read_once(control, &sample, started))
        {
            return false;
        }
        sum += sample;
    }

    *value = (uint16_t)(sum / ADC_AVERAGE_COUNT);
    return true;
}

bool drv_ir_tracking_init(void)
{
    polls = 0u;
    uint32_t started = robot_time_us();

    /* Bring the ADC out of reset */
    clr_w(RESETS_RESET, RESETS_RESET_ADC);
    if (!wait_for_adc_reset(started))
    {
        return false;
    }
    /* Prepare the two sensor inputs */
    configure_adc_pin(IR_RIGHT_PIN);
    configure_adc_pin(IR_LEFT_PIN);
    
    out_w(ADC_CONTROL, ADC_ENABLE);
    return wait_for_adc_ready(started);
}

bool drv_ir_tracking_read(IrRaw *raw)
{
    if (raw == NULL)
    {
        return false;
    }
    polls = 0u;
    uint32_t started = robot_time_us();
    IrRaw pair = {0};

    if (!channel(IR_LEFT_ADC_CHANNEL, &pair.left, started))
    {
        return false;
    }
    if (!channel(IR_RIGHT_ADC_CHANNEL, &pair.right, started))
    {
        return false;
    }
    if (!budget_available(started))
    {
        return false;
    }

    *raw = pair;
    return true;
}
