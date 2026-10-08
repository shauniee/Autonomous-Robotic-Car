/* Executes the real S3 loop with fake GPIO/ADC/S2, without copying its policy. */
#include "task_ir.h"
#include "drv_ir.h"
#include "drv_feedback.h"
#include "../app_program/subsystem2/subsystem2.h"
#include <assert.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>

enum { NORMAL, ADC_FAIL, PUBLISH_FAIL, HEALTH_FAIL, DELAY, RESET_FAIL, INIT_FAIL, INHIBITED };
static jmp_buf finished;
static uint32_t now;
static unsigned scenario, feeds, chirps[4], resets, publications, stops;
static bool stopped;
static bool near(float a,float b) { return fabsf(a-b)<.0001f; }
static bool window(unsigned lo,unsigned hi) { return now>=lo && now<hi; }
ER tk_get_otm(SYSTIM *t) { t->lo=now; return E_OK; }
ER tk_dly_tsk(RELTIM delay) {
    assert(delay>0 && delay<=10);
    if (scenario==DELAY && now==1340) now+=80;
    else now+=delay;
    if (now>(scenario==NORMAL ? 1820u : 1420u)) longjmp(finished,1);
    return E_OK;
}
bool gpio_get_val(unsigned pin) {
    if (pin==21) return !window(1450,1510);
    assert(pin==20);
    return !(window(60,100)||window(660,700)||window(1260,1300)||
             window(1620,1660)||window(1700,1740));
}
bool drv_ir_init(void) { return scenario!=INIT_FAIL; }
bool drv_ir_read(IrRaw *raw) {
    if ((scenario==NORMAL && now>=1760)||(scenario==ADC_FAIL && now>=1380)) return false;
    if (now<620) *raw=(IrRaw){3000,1000};
    else if (now<1240 || window(1360,1400)) *raw=(IrRaw){1000,3000};
    else if (window(1300,1360)) *raw=(IrRaw){1200,1400};
    else *raw=(IrRaw){1800,2200};
    return true;
}
void drv_feedback_init(void) {}
void drv_feedback_chirps(unsigned count,uint32_t at) {
    assert(at==now && count<4); ++chirps[count];
}
void drv_feedback_service(uint32_t at) { assert(at==now); }
void drv_watchdog_enable(void) {}
void drv_watchdog_feed(void) { ++feeds; }
ER s2_stop(void) { stopped=true; ++stops; return E_OK; }
ER s2_reset_stop(void) {
    ++resets;
    if (scenario==RESET_FAIL) return E_OBJ;
    stopped=false; return E_OK;
}
ER s2_health(MotionHealth *h) {
    if (scenario==HEALTH_FAIL && now>=1380) return E_OBJ;
    *h=(MotionHealth){now,now/10+1,stopped||(scenario==INHIBITED && now>=1380)};
    return E_OK;
}
ER s2_publish(MotionRequest r) {
    ++publications;
    assert(r.left>=0 && r.right>=0 && r.left<=.35f && r.right<=.35f);
    if (scenario==PUBLISH_FAIL && now==1380) return E_OBJ;
    if (r.enabled) assert(!stopped && (uint32_t)(now-r.sampled_ms)<=60);
    if (scenario==NORMAL) {
        if (now<1290 || window(1450,1730) || now>=1760) assert(!r.enabled);
        if (window(1360,1420)) assert(r.enabled && near(r.left,.1f) && near(r.right,0));
        if (now==1420) assert(r.enabled && near(r.left,.1f) && near(r.right,.1f));
        if (window(1620,1730)) assert(!r.enabled && r.left==0 && r.right==0);
    } else if (scenario==INIT_FAIL || scenario==RESET_FAIL ||
               (scenario==DELAY && now>=1420) ||
               ((scenario==ADC_FAIL || scenario==HEALTH_FAIL ||
                 scenario==INHIBITED || scenario==PUBLISH_FAIL) && now>=1380)) {
        assert(!r.enabled);
    }
    return E_OK;
}
void tm_printf(const UB *format,...) { (void)format; }

int main(void) {
    for (scenario=NORMAL;scenario<=INHIBITED;++scenario) {
        now=0; feeds=resets=publications=stops=0;
        chirps[0]=chirps[1]=chirps[2]=chirps[3]=0; stopped=true;
        if (!setjmp(finished)) ir_task(0,NULL);
        assert(publications>100);
        if (scenario==INIT_FAIL) assert(feeds==0 && resets==0);
        else {
            assert(chirps[1]==1 && chirps[2]==1 && feeds>0);
            if (scenario==NORMAL) assert(resets==3 && stops>0);
            else assert(stops>0);
        }
    }
    puts("[TEST] INT-05 PASS real S3 loop: recovery enable, stop/reset, ADC/interface failure and missed release");
}
