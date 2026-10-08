#include "subsystem2.h"
#include "drv_motor.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
static unsigned flag,wakes,brakes,applies;
static uint32_t now;
static bool mutex_fails;
static float left,right;
static FP entry;
static jmp_buf end_step;
ER tk_set_flg(ID id,unsigned b) { (void)id;flag|=b;return E_OK; }
ER tk_clr_flg(ID id,unsigned b) { (void)id;flag&=b;return E_OK; }
ER tk_ref_flg(ID id,T_RFLG *f) { (void)id;f->flgptn=flag;return E_OK; }
ER tk_wup_tsk(ID id) { assert(id>0);++wakes;return E_OK; }
ER tk_can_wup(ID id) { (void)id;return E_OK; }
ER tk_loc_mtx(ID id,TMO t) { (void)id;assert(t==2);return mutex_fails?-50:E_OK; }
ER tk_unl_mtx(ID id) { (void)id;return E_OK; }
ER tk_get_otm(SYSTIM *t) { t->lo=now;return E_OK; }
ER tk_slp_tsk(TMO t) { assert(t>0 && t<=20);longjmp(end_step,1); }
ID tk_cre_mtx(const T_CMTX *m) { assert(m->mtxatr==TA_INHERIT);return 1; }
ID tk_cre_flg(const T_CFLG *f) { flag=f->iflgptn;return 2; }
ID app_start_task(FP f,int p,UW core,const char *n) { (void)n;assert(p==13 && core==2);entry=f;return 3; }
void drv_motor_init(void) { ++brakes; }
void drv_motor_brake(void) { ++brakes;left=right=0; }
void drv_motor_apply(float l,float r) { ++applies;left=l;right=r; }
static void step(uint32_t time) { now=time;if (!setjmp(end_step)) entry(0,NULL); }
int main(void) {
    assert(subsystem2_init()==E_OK && flag==1);
    assert(subsystem2_start()==E_OK);
    MotionRequest r={.1f,.2f,100,true};
    assert(s2_publish(r)==E_OK);step(100);assert(left==0 && applies==0); /* Boot latch. */
    assert(s2_reset_stop()==E_OK);step(100);assert(left==0); /* Old command cleared. */
    assert(s2_publish(r)==E_OK);step(160);
#if APP_TEST
    assert(left==0 && right==0 && applies==0);
#else
    assert(left==.1f && right==.2f && applies==1);
#endif
    step(161);assert(left==0 && flag==1); /* Acquisition age, not publish time. */
    r.sampled_ms=162;assert(s2_publish(r)==E_OK);step(162);assert(left==0);
    assert(s2_reset_stop()==E_OK);assert(s2_publish(r)==E_OK);step(162);
    unsigned old_wakes=wakes;
    mutex_fails=true; /* Independent stop must work even when snapshot lock fails. */
    assert(s2_stop()==E_OK && wakes==old_wakes+1 && flag==1);
    step(163);assert(left==0);
    assert(s2_publish(r)<E_OK && flag==1);
    mutex_fails=false;
    MotionHealth h={0};assert(s2_health(&h)==E_OK && h.stopped && h.sequence>=4);
    assert(s2_reset_stop()==E_OK);r.left=.36f;assert(s2_publish(r)==E_OK);
    step(164);assert(left==0 && flag==1 && brakes>0);
    puts("Real Motion service with fake kernel: boot/stop latch, reset invalidation, expiry, failed mutex and TEST inhibition passed.");
}
