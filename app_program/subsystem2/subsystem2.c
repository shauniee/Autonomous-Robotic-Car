#include "subsystem2.h"
#include "drv_motor.h"
#include "drv_encoder.h"
#include "../common/robot_data.h"
#include "../common/robot_snapshot.h"
#include "../common/robot_time.h"

static ID command_mutex, stop_flag, motion_tid;
static MotionCommand latest;
static rd_word_t feedback;		/* single writer: Motion */
static MotionHealth progress;		/* guarded by command_mutex */
static uint32_t stop_requests;		/* incremented by core-1 stop callers inside DI */
static uint32_t serviced_stops;		/* written by Motion after braking */
static uint8_t next_follow_id;

#define STOP_BIT 1u
#define INHIBIT_BIT 2u

static UW control_enter(void)
{
    UW state;
    DI(state);
    return state;
}

static void control_leave(UW state)
{
    EI(state);
}

/* Nonwaiting flag operation only; callers wake Motion after restoring IRQs. */
static ER latch_stop(void)
{
    ++stop_requests;
    return tk_set_flg(stop_flag, STOP_BIT);
}

ER s2_stop(void)
{
    if (stop_flag <= 0) return E_OBJ;
    UW state = control_enter();
    ER er = latch_stop();
    control_leave(state);
    if (motion_tid > 0) tk_wup_tsk(motion_tid);
    return er;
}

ER s2_command(const MotionCommand *command)
{
    if (command_mutex <= 0) return E_OBJ;
    ER er = rd_snapshot_copy(command_mutex, &latest, command, sizeof latest);
    if (er < E_OK) s2_stop();
    return er;
}

uint32_t s2_feedback(void)
{
    return rd_word_read(&feedback);
}

/* Bit 0 = stop latched; bit 1 = startup inhibit (never cleared at runtime).
   Nonwaiting kernel flag operation, safe from any core including startup. */
ER s2_inhibit_startup(void)
{
    if (stop_flag <= 0) return E_OBJ;
    return tk_set_flg(stop_flag, STOP_BIT | INHIBIT_BIT);
}

ER s2_reset_stop(void)
{
    ER er = tk_loc_mtx(command_mutex, 2);
    if (er < E_OK) return er;
    UW state = control_enter();
    T_RFLG flag = {0};
    er = tk_ref_flg(stop_flag, &flag);
    if (er >= E_OK)
    {
        if ((flag.flgptn & INHIBIT_BIT) != 0u)
        {
            er = E_OBJ; /* Startup inhibit: GP20 cannot clear it. */
        }
        else if ((flag.flgptn & STOP_BIT) == 0u || serviced_stops != stop_requests)
        {
            er = E_OBJ; /* Not stopped, or the brake is not yet serviced. */
        }
        else
        {
            er = tk_clr_flg(stop_flag, ~STOP_BIT);
            if (er >= E_OK) latest = (MotionCommand){0};
        }
    }
    control_leave(state);
    ER unlock = tk_unl_mtx(command_mutex);
    return er < E_OK ? er : unlock;
}

ER s2_health(MotionHealth *h)
{
    ER er = tk_loc_mtx(command_mutex, 2);
    if (er < E_OK) return er;
    *h = progress;
    tk_unl_mtx(command_mutex);
    UW state = control_enter();
    T_RFLG f;
    er = tk_ref_flg(stop_flag, &f);
    h->stopped = er < E_OK || (f.flgptn & STOP_BIT) != 0;
    h->brake_acknowledged = er >= E_OK && h->stopped && serviced_stops == stop_requests;
    control_leave(state);
    return er;
}

ER s2_publish(MotionRequest request)
{
    /* Duty pair -> speed fraction and steering; sample time is kept. */
    float sum = request.left + request.right;
    MotionCommand command = {.mode = MOTION_MODE_STOP, .sampled_ms = request.sampled_ms};
    if (request.enabled && sum > 0.0f)
    {
        command.mode = MOTION_MODE_FOLLOW;
        command.speed_mmps = MOTION_SPEED_MAX_MMPS * (sum * 0.5f / MOTION_MAX_DUTY);
        command.steering = (request.left - request.right) / sum;
        command.action_id = ++next_follow_id & rd_mask(RD_MOTION_ACTION_BITS);
        if (command.action_id == 0u) command.action_id = 1u;
    }
    return s2_command(&command);
}

static void motion_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    MotionController controller;
    motion_control_reset(&controller);
    uint32_t release = robot_time_ms();
    /* The GPIO dispatcher is core-1 only, so the encoder starts here. */
    if (!drv_encoder_init()) rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER);
    uint32_t seen_invalid_edges = drv_encoder_invalid_edges();
    for (;;)
    {
        /* Motion is the sole writer of its fault byte: the ISR only counts. */
        uint32_t invalid_edges = drv_encoder_invalid_edges();
        if (invalid_edges != seen_invalid_edges)
        {
            seen_invalid_edges = invalid_edges;
            rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER);
        }

        MotionCommand command = {0};
        ER er = rd_snapshot_copy(command_mutex, &command, &latest, sizeof command);

        /* Stop callers share this core: masking IRQs excludes them from the
           final stop check through the output. Only nonwaiting flag/time reads
           and bounded BSP writes belong in this section. */
        UW irq = control_enter();
        T_RFLG f = {0};
        ER flag_er = tk_ref_flg(stop_flag, &f);
        uint32_t observed_stops = stop_requests;
        bool stopped = flag_er < E_OK || (f.flgptn & STOP_BIT) != 0u;
        uint32_t now = robot_time_ms();
        MotionInput in = {.now_ms = now,
                          .count = {drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT)}};
        MotionOutput out;
        if (stopped || er < E_OK)
        {
            motion_control_reset(&controller);
            out = (MotionOutput){.status = RD_MOTION_IDLE};
        }
        else
        {
            motion_control_step(&controller, &command, &in, &out);
        }
        if (er < E_OK && !stopped)
        {
            (void)latch_stop(); /* Lost the command path: stop. */
            stopped = true;
        }
        if (out.stall) rd_fault_set(RD_OWNER_MOTION, RD_FAULT_MOTOR_STALL);

        ER output_er;
#if APP_TEST
        output_er = drv_motor_brake(); /* TEST never permits movement. */
#else
        output_er = (stopped || (out.duty[0] == 0.0f && out.duty[1] == 0.0f))
            ? drv_motor_brake() : drv_motor_apply(out.duty[0], out.duty[1]);
#endif
        if (output_er < E_OK)
        {
            (void)latch_stop();
            (void)drv_motor_brake();
        }
        else if (stopped)
        {
            serviced_stops = observed_stops; /* Brake serviced for every stop seen. */
        }
        control_leave(irq);

        rd_word_publish(&feedback, rd_motion_feedback_pack(out.status, out.action_id, 0));
        if (output_er >= E_OK)
        {
            uint32_t serviced_ms = robot_time_ms();
            if (tk_loc_mtx(command_mutex, 2) >= E_OK)
            {
                progress.serviced_ms = serviced_ms;
                ++progress.sequence;
                tk_unl_mtx(command_mutex);
            }
            else
            {
                (void)drv_motor_brake();
                s2_stop();
            }
        }
        /* Fixed-rate release; a stop wake-up services immediately without shifting it. */
        tk_slp_tsk(robot_time_next_delay(&release, MOTION_PERIOD_MS, robot_time_ms()));
        tk_can_wup(TSK_SELF);
    }
}

ER subsystem2_init(void)
{
    ER er = drv_motor_init();
    if (er < E_OK) return er;
    T_CMTX m = {0};
    m.mtxatr = TA_INHERIT;
    command_mutex = tk_cre_mtx(&m);
    if (command_mutex < E_OK) return command_mutex;
    T_CFLG f = {0};
    f.iflgptn = STOP_BIT; /* Boot latched: motion needs a deliberate reset. */
    stop_flag = tk_cre_flg(&f);
    return stop_flag < E_OK ? stop_flag : E_OK;
}

ER subsystem2_start(void)
{
    motion_tid = app_start_task((FP)motion_task, S2_PRI_MOTION, TP_PRC2, "Motion");
    return motion_tid < E_OK ? motion_tid : E_OK;
}
