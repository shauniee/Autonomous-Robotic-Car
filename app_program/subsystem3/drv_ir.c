#include "drv_ir_io.h"
#include "drv_ir.h"
#define ADC_REG_BASE 0x4004c000u
#define ADC_CONTROL ADC_REG_BASE
#define ADC_RESULT (ADC_REG_BASE+4)
#define ADC_READY (1u<<8)
#define ADC_ERROR (1u<<9)
/* Bound the WHOLE paired acquisition, including discarded conversions. */
#define ADC_BUDGET_US 2000u
/* Independent finite poll budget also fails closed if the timer stalls. */
static unsigned polls;
static uint32_t clock_us(void) { return in_w(TIMER_TIMERAWL); }
static bool ready(uint32_t started) {
    while (!(in_w(ADC_CONTROL)&ADC_READY)) {
        if (++polls>=4096 || (uint32_t)(clock_us()-started)>=ADC_BUDGET_US) return false;
    }
    return (uint32_t)(clock_us()-started)<ADC_BUDGET_US;
}
bool drv_ir_init(void) {
    polls=0;
    clr_w(RESETS_RESET,RESETS_RESET_ADC);
    uint32_t started=clock_us();
    while (!(in_w(RESETS_RESET_DONE)&RESETS_RESET_ADC))
        if (++polls>=4096 || (uint32_t)(clock_us()-started)>=ADC_BUDGET_US) return false;
    for (unsigned p=26;p<=27;++p) {
        out_w(GPIO_CTRL(p),31); /* NULL mux; analogue pad, no digital pulls. */
        clr_w(GPIO(p),GPIO_IE|GPIO_PUE|GPIO_PDE);
        set_w(GPIO(p),GPIO_OD);
    }
    out_w(ADC_CONTROL,1);
    return ready(started);
}
static bool channel(unsigned mux, uint16_t *value, uint32_t started) {
    out_w(ADC_CONTROL,1|(mux<<12)|ADC_ERROR);
    uint32_t sum=0;
    for (unsigned n=0;n<9;++n) {
        out_w(ADC_CONTROL,1|(mux<<12)|(1u<<2)|ADC_ERROR);
        if (!ready(started) || (in_w(ADC_CONTROL)&ADC_ERROR)) return false;
        uint32_t raw=in_w(ADC_RESULT)&4095;
        if (n) sum+=raw; /* First conversion after mux switch is discarded. */
    }
    *value=(uint16_t)(sum/8); return true;
}
bool drv_ir_read(IrRaw *raw) {
    polls=0;
    IrRaw pair; uint32_t started=clock_us();
    if (!channel(0,&pair.left,started) || !channel(1,&pair.right,started)) return false;
    *raw=pair; return true;
}
