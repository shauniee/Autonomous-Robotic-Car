#include "subsystem2.h"
#include "drv_motor.h"
#include "drv_encoder.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
static ID request_mutex, stop_flag, motion_tid;
static MotionRequest latest;
static MotionHealth progress;
/* All runtime accesses are serialized by core-1 PRIMASK sections. */
static uint32_t control_epoch = 1u, brake_epoch;
static bool brake_acknowledged;

static UW control_enter(void)
{
    UW state;
    DI(state);
    __asm__ volatile ("" ::: "memory");
    return state;
}

static void control_leave(UW state)
{
    __asm__ volatile ("" ::: "memory");
    EI(state);
    __asm__ volatile ("" ::: "memory");
}

/* Nonwaiting flag operation only; callers wake Motion after restoring IRQs. */
static ER latch_stop(void)
{
    ++control_epoch;
    brake_acknowledged = false;
    return tk_set_flg(stop_flag, 1u);
}
ER s2_stop(void) {
    if (stop_flag<=0) return E_OBJ;
    UW state = control_enter();
    ER er = latch_stop();
    control_leave(state);
    if (motion_tid>0) tk_wup_tsk(motion_tid);
    return er;
}
ER s2_publish(MotionRequest request) {
    ER er=tk_loc_mtx(request_mutex,2);
    if (er<E_OK) { s2_stop(); return er; }
    latest=request;
    return tk_unl_mtx(request_mutex);
}
/* Bit 0 = stop latched; bit 1 = startup inhibit (never cleared at runtime).
   Nonwaiting kernel flag operation: sets the bits and touches no motor
   output, so it is safe from any core, including startup on core 0. */
ER s2_inhibit_startup(void)
{
    if (stop_flag <= 0) return E_OBJ;
    return tk_set_flg(stop_flag, 3u);
}

ER s2_reset_stop(void)
{
    ER er = tk_loc_mtx(request_mutex, 2);
    if (er < E_OK)
    {
        return er;
    }
    UW state = control_enter();
    T_RFLG flag = {0};
    er = tk_ref_flg(stop_flag, &flag);
    if (er >= E_OK)
    {
        if ((flag.flgptn & 2u) != 0u)
        {
            er = E_OBJ; /* Startup inhibit: GP20 cannot clear it. */
        }
        else if (((flag.flgptn & 1u) == 0u) || !brake_acknowledged ||
            (brake_epoch != control_epoch))
        {
            er = E_OBJ;
        }
        else
        {
            er = tk_clr_flg(stop_flag, 0u);
            if (er >= E_OK)
            {
                latest = (MotionRequest){0};
                ++control_epoch; /* Invalidate work copied before this reset. */
                brake_acknowledged = false;
            }
        }
    }
    control_leave(state);
    ER unlock_er = tk_unl_mtx(request_mutex);
    return er < E_OK ? er : unlock_er;
}
ER s2_health(MotionHealth *h) {
    ER er=tk_loc_mtx(request_mutex,2);
    if (er<E_OK) return er;
    *h=progress;
    tk_unl_mtx(request_mutex);
    UW state = control_enter();
    T_RFLG f;
    er=tk_ref_flg(stop_flag,&f);
    h->stopped=er<E_OK || (f.flgptn&1)!=0;
    h->brake_acknowledged = er >= E_OK && h->stopped &&
        brake_acknowledged && (brake_epoch == control_epoch);
    control_leave(state);
    return er;
}
static void motion_task(INT stacd, void *exinf) {
    (void)stacd; (void)exinf;
    uint32_t next=robot_time_ms();
    /* The GPIO dispatcher is core-1 only, so the encoder starts here. */
    if (!drv_encoder_init()) rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER);
    uint32_t seen_invalid_edges = drv_encoder_invalid_edges();
    for (;;) {
        uint32_t now=robot_time_ms();
        /* Motion is the sole writer of its fault byte: the ISR only counts. */
        uint32_t invalid_edges = drv_encoder_invalid_edges();
        if (invalid_edges != seen_invalid_edges) {
            seen_invalid_edges = invalid_edges;
            rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER);
        }
        MotionRequest r={0}; T_RFLG f={0};
        ER er=tk_loc_mtx(request_mutex,2);
        uint32_t request_epoch;
        UW copy_state = control_enter();
        request_epoch = control_epoch;
        if (er >= E_OK)
        {
            r = latest;
        }
        control_leave(copy_state);
        if (er >= E_OK)
        {
            er = tk_unl_mtx(request_mutex);
        }
        /* Runtime stop callers must share Motion's core 1. Masking its IRQs
           excludes task dispatch through the final stop check and output.
           Only nonwaiting flag/time reads and bounded BSP writes belong here;
           never acquire a mutex, sleep, or wake a task in this section. */
        UW interrupt_state;
        interrupt_state = control_enter();
        ER flag_er = tk_ref_flg(stop_flag, &f);
        bool stopped = flag_er < E_OK ||
                       (f.flgptn & 1u) != 0u;
        bool same_epoch = request_epoch == control_epoch;
        bool valid = er >= E_OK && same_epoch &&
                     motion_request_valid(r, stopped, robot_time_ms());
        ER output_er;
        if (!valid)
        {
            /* Old copied work is discarded; it must not stop a new run. */
            if (!stopped && ((er < E_OK) || (same_epoch && r.enabled)))
            {
                (void)latch_stop();
                stopped = true;
            }
            output_er = drv_motor_brake();
        }
        else
        {
#if APP_TEST
            output_er = drv_motor_brake(); /* TEST never permits movement. */
#else
            output_er = drv_motor_apply(r.left, r.right);
#endif
        }
        if (output_er < E_OK)
        {
            (void)latch_stop();
            (void)drv_motor_brake();
        }
        else
        {
            brake_acknowledged = !valid && stopped && flag_er >= E_OK;
            brake_epoch = control_epoch;
        }
        control_leave(interrupt_state);
        if (output_er >= E_OK)
        {
            uint32_t serviced_ms = robot_time_ms();
            er = tk_loc_mtx(request_mutex, 2);
            if (er >= E_OK)
            {
                progress.serviced_ms = serviced_ms;
                ++progress.sequence;
                tk_unl_mtx(request_mutex);
            }
            else
            {
                (void)drv_motor_brake();
                s2_stop();
            }
        }
        /* Fixed deadline; stop wake-ups service immediately without shifting it. */
        now=robot_time_ms();
        if ((int32_t)(now-next)>=0) {
            next+=20;
            if ((int32_t)(now-next)>=0) next=now+20;
        }
        tk_slp_tsk((TMO)(next-now));
        tk_can_wup(TSK_SELF);
    }
}
ER subsystem2_init(void) {
    ER er = drv_motor_init();
    if (er < E_OK)
    {
        return er;
    }
    T_CMTX m={0}; m.mtxatr=TA_INHERIT;
    request_mutex=tk_cre_mtx(&m); if (request_mutex<E_OK) return request_mutex;
    T_CFLG f={0}; f.iflgptn=1;
    stop_flag=tk_cre_flg(&f); return stop_flag<E_OK ? stop_flag : E_OK;
}
ER subsystem2_start(void) {
    motion_tid=app_start_task((FP)motion_task,S2_PRI_MOTION,TP_PRC2,"Motion");
    return motion_tid<E_OK ? motion_tid : E_OK;
}
