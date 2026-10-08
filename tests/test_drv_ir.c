#include "drv_ir.h"
#include "hardware_io.h"
#include <assert.h>
#include <stdio.h>
static uint32_t time_us, reads, mux, conversions[2], adc_polls;
static bool stalled, error;
uint32_t in_w(uint32_t a) {
    if (a==TIMER_TIMERAWL) return stalled ? 0 : time_us++;
    if (a==RESETS_RESET_DONE) return 1;
    if (a==0x4004c000u) {
        ++adc_polls;
        return stalled ? 0 : (1u<<8)|(error?(1u<<9):0);
    }
    if (a==0x4004c004u) {
        unsigned n=conversions[mux]++;
        ++reads;
        return n==0 ? 4095 : 1000u*(mux+1)+n-1;
    }
    return 0;
}
void out_w(uint32_t a,uint32_t v) { if (a==0x4004c000u) mux=(v>>12)&7; }
void clr_w(uint32_t a,uint32_t b) { (void)a;(void)b; }
void set_w(uint32_t a,uint32_t b) { (void)a;(void)b; }
int main(void) {
    assert(drv_ir_init());
    IrRaw r={0};
    assert(drv_ir_read(&r));
    assert(reads==18 && conversions[0]==9 && conversions[1]==9);
    assert(r.left==1003 && r.right==2003);
    error=true; assert(!drv_ir_read(&r)); assert(r.left==1003);
    error=false; stalled=true; adc_polls=0;
    assert(!drv_ir_read(&r) && adc_polls<=4097);
    assert(r.right==2003); /* A partial/failed pair never escapes. */
    assert(!drv_ir_init());
    puts("Subsystem ADC: mux discard, eight-sample averages, errors and stalled-clock bound passed.");
}
