#include "subsystem2.h"
#include "drv_motor.h"
#include "drv_encoder.h"
#include "motion_stop.h"
#include "../common/buttons.h"
#include "../common/robot_data.h"
#include "../common/robot_snapshot.h"
#include "../common/robot_time.h"

static ID command_mutex, stop_flag, motion_tid;
static MotionCommand latest;
static rd_word_t feedback;		/* single writer: Motion */
static MotionHealth progress;		/* guarded by command_mutex */
static uint32_t stop_requests;		/* incremented by core-1 stop callers inside DI */
static uint32_t serviced_stops;		/* written by Motion after braking */

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
    if (command == NULL)
    {
        (void)s2_stop();
        return E_PAR;
    }
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
    if (command_mutex <= 0 || stop_flag <= 0)
    {
        return E_OBJ;
    }
    ER er = tk_loc_mtx(command_mutex, 2);
    if (er < E_OK) return er;
    UW state = control_enter();
    T_RFLG flag = {0};
    er = tk_ref_flg(stop_flag, &flag);
    if (er >= E_OK)
    {
        if (!motion_stop_reset_allowed(
                (flag.flgptn & INHIBIT_BIT) != 0u,
                (flag.flgptn & STOP_BIT) != 0u,
                stop_requests, serviced_stops,
                drv_buttons_read().gp21_down ||
                (!APP_NO_GUARDS && rd_fault_stop_pending())))
        {
            er = E_OBJ;
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

static ER health_copy(MotionHealth *h, TMO wait)
{
    if (h == NULL) return E_PAR;
    ER er = tk_loc_mtx(command_mutex, wait);
    if (er < E_OK) return er;
    *h = progress;
    er = tk_unl_mtx(command_mutex);
    if (er < E_OK)
    {
        return er;
    }
    UW state = control_enter();
    T_RFLG f;
    er = tk_ref_flg(stop_flag, &f);
    bool stopped = er < E_OK || (f.flgptn & STOP_BIT) != 0;
    bool braked = er >= E_OK && stopped && serviced_stops == stop_requests;
    h->flags = (uint8_t)((stopped ? MOTION_HEALTH_STOPPED_BIT : 0u) |
                         (braked ? MOTION_HEALTH_BRAKED_BIT : 0u));
    control_leave(state);
    return er;
}

static void motion_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    MotionController controller;
    motion_control_reset(&controller);
    uint32_t release = robot_time_ms();
    /* The GPIO dispatcher is core-1 only, so the encoder starts here. */
    bool encoder_ready = drv_encoder_init();
    if (!encoder_ready) rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER); /* permanent: no ISR */
    uint32_t seen_invalid_edges = drv_encoder_invalid_edges();
    uint32_t good_counts = 0u;
    uint32_t last_left = drv_encoder_count(ENCODER_LEFT), last_right = drv_encoder_count(ENCODER_RIGHT);
    for (;;)
    {
        /* Motion is the sole writer of its fault byte: the ISR only counts. */
        uint32_t invalid_edges = drv_encoder_invalid_edges();
        uint32_t left = drv_encoder_count(ENCODER_LEFT), right = drv_encoder_count(ENCODER_RIGHT);
        if (invalid_edges != seen_invalid_edges)
        {
            seen_invalid_edges = invalid_edges;
            good_counts = 0u;
            rd_fault_set(RD_OWNER_MOTION, RD_FAULT_ENCODER);
        }
        else if (encoder_ready && (rd_fault_get(RD_OWNER_MOTION) & RD_FAULT_ENCODER) != 0u)
        {
            int32_t dl = enc_travel_signed(left, last_left), dr = enc_travel_signed(right, last_right);
            good_counts += (uint32_t)(dl < 0 ? -dl : dl) + (uint32_t)(dr < 0 ? -dr : dr);
            if (good_counts >= ENC_FAULT_CLEAR_COUNTS) rd_fault_clear(RD_OWNER_MOTION, RD_FAULT_ENCODER);
        }
        last_left = left;
        last_right = right;

        /* Check the independent path before attempting a command copy.
           A held stop never waits for the command mutex or diagnostics. */
        T_RFLG initial = {0};
        ER initial_er = tk_ref_flg(stop_flag, &initial);
        bool local_stop = drv_buttons_read().gp21_down ||
            (!APP_NO_GUARDS && rd_fault_stop_pending());
        if (local_stop && initial_er >= E_OK &&
            (initial.flgptn & STOP_BIT) == 0u)
        {
            (void)s2_stop();
        }
        MotionCommand command = {0};
        ER er = E_OK;
        if (!local_stop && initial_er >= E_OK &&
            (initial.flgptn & STOP_BIT) == 0u)
        {
            er = rd_snapshot_copy(command_mutex, &command, &latest,
                sizeof command);
        }

        uint32_t now = robot_time_ms();
        MotionInput in = {.now_ms = now,
            .count = {drv_encoder_count(ENCODER_LEFT),
                      drv_encoder_count(ENCODER_RIGHT)}};
        MotionOutput out = {.status = RD_MOTION_IDLE};
        bool previously_stopped = local_stop || initial_er < E_OK ||
            (initial.flgptn & STOP_BIT) != 0u;
        if (previously_stopped || er < E_OK)
        {
            motion_control_reset(&controller);
        }
        else
        {
            /* Wheel calculations leave interrupts enabled. Only the final
               guard and bounded output writes need the exclusion section. */
            motion_control_step(&controller, &command, &in, &out);
        }
        if (out.stall)
        {
            rd_fault_set(RD_OWNER_MOTION, RD_FAULT_MOTOR_STALL);
        }
        else if (previously_stopped)
        {
            rd_fault_clear(RD_OWNER_MOTION, RD_FAULT_MOTOR_STALL);
        }

        UW irq = control_enter();
        T_RFLG f = {0};
        ER flag_er = tk_ref_flg(stop_flag, &f);
        bool stopped = flag_er < E_OK || (f.flgptn & STOP_BIT) != 0u;
        bool expired = motion_cmd_mode(&command) != MOTION_MODE_STOP &&
            !motion_command_fresh(&command, robot_time_ms());
        bool unsafe = drv_buttons_read().gp21_down ||
            (!APP_NO_GUARDS && rd_fault_stop_pending()) ||
            initial_er < E_OK || er < E_OK || expired ||
            out.status == RD_MOTION_FAILED;
        if ((unsafe && !stopped) || flag_er < E_OK)
        {
            (void)latch_stop();
            stopped = true;
        }
        uint32_t observed_stops = stop_requests;
        if (stopped)
        {
            motion_control_reset(&controller);
            out.duty[0] = 0.0f;
            out.duty[1] = 0.0f;
            if (out.status == RD_MOTION_RUNNING)
            {
                out.status = RD_MOTION_IDLE;
            }
        }

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
        if (out.stall)
        {
            /* Outputs have already been braked; report before this fault clears. */
            tm_printf((UB *)"[MOTION] motor stall: mode=%u action=%u "
                "speed_mmps=%u steering_q=%d left=%u right=%u\n",
                (unsigned)motion_cmd_mode(&command),
                (unsigned)motion_cmd_action(&command),
                (unsigned)motion_cmd_speed(&command),
                (int)motion_cmd_steering(&command),
                (unsigned)in.count[0], (unsigned)in.count[1]);
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
    stop_requests = 1u; /* Boot STOP is not acknowledged until Motion brakes. */
    serviced_stops = 0u;
    T_CFLG f = {0};
    f.iflgptn = STOP_BIT | (APP_TEST ? INHIBIT_BIT : 0u);
    stop_flag = tk_cre_flg(&f);
    return stop_flag < E_OK ? stop_flag : E_OK;
}

ER subsystem2_start(void)
{
    motion_tid = app_start_task((FP)motion_task, S2_PRI_MOTION, TP_PRC2, "Motion");
    return motion_tid < E_OK ? motion_tid : E_OK;
}

ER s2_health(MotionHealth *health)
{
    return health_copy(health, 2);
}

ER s2_health_try(MotionHealth *health)
{
    return health_copy(health, TMO_POL);
}
