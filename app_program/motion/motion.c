/* Use the port guard when mixing ISO C headers with tk/syslib.h. */
#define PROHIBIT_DEF_SIZE_T
#include <math.h>
#include <stdatomic.h>
#include <string.h>
#ifndef HOST_TEST
#include "../app.h"
#endif
#include "motion.h"

#ifndef HOST_TEST
#include "../common/buttons.h"
#include "../common/drv_gpio_irq.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#endif

#ifndef HOST_TEST
/* ---- Motion task ---- */
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

ER motion_stop(void)
{
    if (stop_flag <= 0) return E_OBJ;
    UW state = control_enter();
    ER er = latch_stop();
    control_leave(state);
    if (motion_tid > 0) tk_wup_tsk(motion_tid);
    return er;
}

/* Copies the shared command under the mutex; dst is untouched if the lock fails. */
static ER command_copy(MotionCommand *dst, const MotionCommand *src)
{
    ER er = tk_loc_mtx(command_mutex, RD_SNAPSHOT_LOCK_MS);
    if (er < E_OK) return er;
    *dst = *src;
    return tk_unl_mtx(command_mutex);
}

ER motion_command(const MotionCommand *command)
{
    if (command == NULL)
    {
        (void)motion_stop();
        return E_PAR;
    }
    if (command_mutex <= 0) return E_OBJ;
    ER er = command_copy(&latest, command);
    if (er < E_OK) motion_stop();
    return er;
}

uint32_t motion_feedback(void)
{
    return rd_word_read(&feedback);
}

/* Bit 0 = stop latched; bit 1 = startup inhibit (never cleared at runtime).
   Nonwaiting kernel flag operation, safe from any core including startup. */
ER motion_inhibit_startup(void)
{
    if (stop_flag <= 0) return E_OBJ;
    return tk_set_flg(stop_flag, STOP_BIT | INHIBIT_BIT);
}

ER motion_reset_stop(void)
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
                rd_fault_stop_pending()))
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
            rd_fault_stop_pending();
        if (local_stop && initial_er >= E_OK &&
            (initial.flgptn & STOP_BIT) == 0u)
        {
            (void)motion_stop();
        }
        MotionCommand command = {0};
        ER er = E_OK;
        if (!local_stop && initial_er >= E_OK &&
            (initial.flgptn & STOP_BIT) == 0u)
        {
            er = command_copy(&command, &latest);
        }

        uint32_t now = robot_time_ms();
        MotionInput in = {.now_ms = now,
            .count = {drv_encoder_count(ENCODER_LEFT),
                      drv_encoder_count(ENCODER_RIGHT)}};
        /* Out-of-range trace speed encodes unavailable, never measured zero. */
        MotionOutput out = {.status = RD_MOTION_IDLE,
            .measured_cps = {-32768.0f, -32768.0f}};
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
            rd_fault_stop_pending() ||
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

        /* Encode only after output service, outside the IRQ exclusion. */
        mqtt_wheel_trace_t trace = {
            .sampled_ms = in.now_ms,
            .inhibited = APP_TEST != 0,
            .output_ok = output_er >= E_OK};
        for (unsigned w = 0u; w < 2u; ++w)
        {
            trace.target_cps[w] = out.target_cps[w];
            trace.measured_cps[w] = out.measured_cps[w];
            trace.duty[w] = APP_TEST ? 0.0f : out.duty[w];
        }
        (void)motion_trace_publish(&trace);

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
                motion_stop();
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

ER motion_init(void)
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

ER motion_start(void)
{
    motion_tid = app_start_task((FP)motion_task, MOTION_PRI, TP_PRC2, "Motion");
    return motion_tid < E_OK ? motion_tid : E_OK;
}

ER motion_health(MotionHealth *health)
{
    return health_copy(health, 2);
}

ER motion_health_try(MotionHealth *health)
{
    return health_copy(health, TMO_POL);
}

/* ---- Motor driver ---- */
/* Template clk_sys is 125 MHz. PWM DIV is 8.4 fixed point:
   125 MHz / 6.25 / 1000 = 20 kHz, carrier maximum. */
/* Verify motor lead polarity on the actual robot, wheels raised. */

static ER motor_gpio_low(void)
{
    ER result = E_OK;
    for (unsigned pin = 8u; pin <= 11u; ++pin)
    {
        ER er = gpio_set_val(pin, 0u);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
        er = gpio_set_pin(pin, GPIO_MODE_OUT);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
    }
    return result;
}

ER drv_motor_brake(void)
{
    ER result = E_OK;
    /* Attempt every channel even if an earlier write fails. */
    for (unsigned pin = 8u; pin <= 11u; ++pin)
    {
        ER er = pwm_set_cc(pin, 0u);
        if ((er < E_OK) && (result >= E_OK))
        {
            result = er;
        }
    }
    if (result < E_OK)
    {
        /* Best effort SIO fallback; retain the first PWM error. */
        (void)motor_gpio_low();
    }
    return result;
}

static ER motor_setup_failed(ER error)
{
    (void)drv_motor_brake();
    /* Partial setup may leave some pins outside the PWM function. */
    (void)motor_gpio_low();
    return error;
}

static ER motor_setup_slice(unsigned pin)
{
    ER er = pwm_set_enabled(pin, FALSE);
    if (er >= E_OK)
    {
        out_w(PWM_BASE + (pin / 2u) * 0x14u + PWM_CHx_DIV,
              MOTOR_DIV_8_4);
        out_w(PWM_BASE + (pin / 2u) * 0x14u + PWM_CHx_CTR, 0u);
        er = pwm_set_wrap(pin, MOTOR_WRAP);
    }
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin, 0u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin + 1u, 0u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_pin(pin);
    }
    if (er >= E_OK)
    {
        er = pwm_set_pin(pin + 1u);
    }
    if (er >= E_OK)
    {
        er = pwm_set_enabled(pin, TRUE);
    }
    return er;
}

ER drv_motor_init(void)
{
    /* LOW/LOW is the carrier brake state; establish it before PWM mux. */
    ER er = motor_gpio_low();
    if (er >= E_OK)
    {
        /* Release only; never reset the shared block or another slice. */
        clr_w(RESETS_RESET, RESETS_RESET_PWM);
        for (unsigned pin = 8u; (pin <= 10u) && (er >= E_OK); pin += 2u)
        {
            er = motor_setup_slice(pin);
        }
    }
    if (er < E_OK)
    {
        er = motor_setup_failed(er);
    }
    return er;
}

/* Signed duty: positive drives the wheel forward, negative backward.
   forward_a is the per-motor polarity constant (robot_config.h): the channel
   that moves the wheel forward. Magnitude is clamped to MOTION_MAX_DUTY. */
static ER motor_wheel(unsigned pin, float duty, int forward_a)
{
    float magnitude = duty < 0.0f ? -duty : duty;
    if (magnitude > MOTION_MAX_DUTY)
    {
        magnitude = MOTION_MAX_DUTY;
    }
    /* Clamped magnitude <= MOTION_MAX_DUTY keeps compare within MOTOR_WRAP. */
    UW compare = (UW)(magnitude * (MOTOR_WRAP + 1u));
    int a_active = (duty >= 0.0f) ? (forward_a != 0) : (forward_a == 0);
    ER er = pwm_set_cc(pin, a_active ? compare : 0u);
    if (er >= E_OK)
    {
        er = pwm_set_cc(pin + 1u, a_active ? 0u : compare);
    }
    return er;
}

ER drv_motor_apply(float left, float right)
{
    ER er = E_PAR;
    /* Reject non-finite values before writing either wheel. */
    if (isfinite(left) && isfinite(right))
    {
        er = motor_wheel(8u, left, LEFT_FORWARD_A);
        if (er >= E_OK)
        {
            er = motor_wheel(10u, right, RIGHT_FORWARD_A);
        }
    }
    if (er < E_OK)
    {
        (void)drv_motor_brake();
    }
    return er;
}

/* ---- Encoder driver ---- */
/* Channel pins per wheel: [0] = A (high bit of the level), [1] = B. */
static const unsigned pins[ENCODER_WHEELS][2] = {
    {ENC_LEFT_A_PIN, ENC_LEFT_B_PIN},
    {ENC_RIGHT_A_PIN, ENC_RIGHT_B_PIN},
};
static const int sign[ENCODER_WHEELS] = {LEFT_ENCODER_SIGN, RIGHT_ENCODER_SIGN};

/* Each storage unit has one writer: the dispatcher handler on core 1. */
static _Atomic uint32_t counts[ENCODER_WHEELS];
static _Atomic uint32_t invalid_edges;
static unsigned previous[ENCODER_WHEELS];	/* ISR-private */

static unsigned level(unsigned wheel)
{
    return (unsigned)gpio_get_val(pins[wheel][0]) * 2u + (unsigned)gpio_get_val(pins[wheel][1]);
}

static void edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)sampled_us;
    unsigned wheel = (pin == ENC_LEFT_A_PIN || pin == ENC_LEFT_B_PIN) ? ENCODER_LEFT : ENCODER_RIGHT;
    unsigned current = level(wheel);
    int delta = enc_quad_step(previous[wheel], current);
    previous[wheel] = current;
    if (delta == ENC_QUAD_INVALID || edges == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)) {
        uint32_t n = atomic_load_explicit(&invalid_edges, memory_order_relaxed);
        atomic_store_explicit(&invalid_edges, n + 1u, memory_order_release);
    } else if (delta != 0) {
        uint32_t c = atomic_load_explicit(&counts[wheel], memory_order_relaxed);
        atomic_store_explicit(&counts[wheel], enc_advance(c, delta * sign[wheel]), memory_order_release);
    }
}

bool drv_encoder_init(void)
{
    if (drv_gpio_irq_init() < E_OK) {
        return false;
    }
    for (unsigned wheel = 0u; wheel < ENCODER_WHEELS; ++wheel) {
        for (unsigned channel = 0u; channel < 2u; ++channel) {
            unsigned pin = pins[wheel][channel];
            if (gpio_set_pin(pin, GPIO_MODE_IN) < E_OK || !drv_gpio_irq_claim(pin, edge)) {
                return false;
            }
        }
        previous[wheel] = level(wheel);
    }
    for (unsigned wheel = 0u; wheel < ENCODER_WHEELS; ++wheel) {
        for (unsigned channel = 0u; channel < 2u; ++channel) {
            if (!drv_gpio_irq_enable(pins[wheel][channel], true)) {
                return false;
            }
        }
    }
    return true;
}

uint32_t drv_encoder_count(encoder_wheel_t wheel)
{
    if ((unsigned)wheel >= ENCODER_WHEELS) {
        return 0u;
    }
    return atomic_load_explicit(&counts[wheel], memory_order_acquire);
}

uint32_t drv_encoder_invalid_edges(void)
{
    return atomic_load_explicit(&invalid_edges, memory_order_acquire);
}
#endif /* !HOST_TEST */

/* ---- Wheel trace ring ---- */
enum { TRACE_DEPTH = 8u };
static uint8_t records[TRACE_DEPTH][MQTT_LEN_WHEEL_TRACE];
static _Atomic uint32_t head, tail;

bool motion_trace_publish(const mqtt_wheel_trace_t *trace)
{
    uint32_t h = atomic_load_explicit(&head, memory_order_relaxed);
    if (h - atomic_load_explicit(&tail, memory_order_acquire) >= TRACE_DEPTH)
    {
        return false;
    }
    if (mqtt_encode_wheel_trace(records[h % TRACE_DEPTH],
            MQTT_LEN_WHEEL_TRACE, trace) == 0u)
    {
        return false;
    }
    atomic_store_explicit(&head, h + 1u, memory_order_release);
    return true;
}

unsigned motion_trace_take(uint8_t *out, unsigned capacity)
{
    uint32_t t = atomic_load_explicit(&tail, memory_order_relaxed);
    if (out == NULL || capacity < MQTT_LEN_WHEEL_TRACE ||
        t == atomic_load_explicit(&head, memory_order_acquire))
    {
        return 0u;
    }
    memcpy(out, records[t % TRACE_DEPTH], MQTT_LEN_WHEEL_TRACE);
    atomic_store_explicit(&tail, t + 1u, memory_order_release);
    return MQTT_LEN_WHEEL_TRACE;
}
