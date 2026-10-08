#include "subsystem2.h"
#include "drv_motor.h"
static ID request_mutex, stop_flag, motion_tid;
static MotionRequest latest;
static MotionHealth progress;
static uint32_t now_ms(void) { SYSTIM t; tk_get_otm(&t); return t.lo; }
ER s2_stop(void) {
    if (stop_flag<=0) return E_OBJ;
    ER er=tk_set_flg(stop_flag,1);
    if (motion_tid>0) tk_wup_tsk(motion_tid);
    return er;
}
ER s2_publish(MotionRequest request) {
    ER er=tk_loc_mtx(request_mutex,2);
    if (er<E_OK) { s2_stop(); return er; }
    latest=request;
    return tk_unl_mtx(request_mutex);
}
ER s2_reset_stop(void) {
    ER er=tk_loc_mtx(request_mutex,2);
    if (er<E_OK) return er;
    latest=(MotionRequest){0}; /* Reset never resurrects a previous duty. */
    er=tk_clr_flg(stop_flag,0);
    tk_unl_mtx(request_mutex);
    return er;
}
ER s2_health(MotionHealth *h) {
    ER er=tk_loc_mtx(request_mutex,2);
    if (er<E_OK) return er;
    *h=progress;
    tk_unl_mtx(request_mutex);
    T_RFLG f;
    er=tk_ref_flg(stop_flag,&f);
    h->stopped=er<E_OK || (f.flgptn&1)!=0;
    return er;
}
static void motion_task(INT stacd, void *exinf) {
    (void)stacd; (void)exinf;
    uint32_t next=now_ms();
    for (;;) {
        uint32_t now=now_ms();
        MotionRequest r={0}; T_RFLG f={0};
        ER er=tk_loc_mtx(request_mutex,2);
        if (er>=E_OK) { r=latest; tk_unl_mtx(request_mutex); }
        bool stopped=tk_ref_flg(stop_flag,&f)<E_OK || (f.flgptn&1)!=0;
        bool valid=er>=E_OK && motion_request_valid(r,stopped,now);
        if (!valid) {
            drv_motor_brake();
            /* Expiry/bad active request is latched, even if the producer recovers. */
            if (r.enabled || er<E_OK) tk_set_flg(stop_flag,1);
        } else {
#if APP_TEST
            drv_motor_brake(); /* Test firmware never permits motor movement. */
#else
            drv_motor_apply(r.left,r.right);
#endif
        }
        er=tk_loc_mtx(request_mutex,2);
        if (er>=E_OK) {
            progress.serviced_ms=now; ++progress.sequence;
            tk_unl_mtx(request_mutex);
        } else { drv_motor_brake(); tk_set_flg(stop_flag,1); }
        /* Fixed deadline; stop wake-ups service immediately without shifting it. */
        now=now_ms();
        if ((int32_t)(now-next)>=0) {
            next+=20;
            if ((int32_t)(now-next)>=0) next=now+20;
        }
        tk_slp_tsk((TMO)(next-now));
        tk_can_wup(TSK_SELF);
    }
}
ER subsystem2_init(void) {
    drv_motor_init();
    T_CMTX m={0}; m.mtxatr=TA_INHERIT;
    request_mutex=tk_cre_mtx(&m); if (request_mutex<E_OK) return request_mutex;
    T_CFLG f={0}; f.iflgptn=1;
    stop_flag=tk_cre_flg(&f); return stop_flag<E_OK ? stop_flag : E_OK;
}
ER subsystem2_start(void) {
    motion_tid=app_start_task((FP)motion_task,S2_PRI_MOTION,TP_PRC2,"Motion");
    return motion_tid<E_OK ? motion_tid : E_OK;
}
