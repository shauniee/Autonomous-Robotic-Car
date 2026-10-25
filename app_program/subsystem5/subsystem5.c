#include "subsystem5.h"
#include <stdatomic.h>
#include "../common/drv_gpio_irq.h"
#include "../common/robot_time.h"
#include "../common/robot_config.h"
#include "../common/robot_snapshot.h"
#include "../subsystem2/drv_encoder.h"

/* Echo capture state. The handler is the only writer once the task arms the
   measurement with the Echo IRQ disabled, so no field has two writers at once. */
enum { ECHO_IDLE, ECHO_ARMED, ECHO_HIGH, ECHO_DONE };
static _Atomic unsigned echo_state;
static _Atomic uint32_t pulse_us;
static uint32_t rise_us;			/* handler-private */
static rd_word_t front_word;			/* writer: the obstacle task */
static rd_word_t side_word;			/* writer: the obstacle task */
static rd_word_t scan_word;			/* writer: the obstacle task */
static ID profile_mutex;
static ObstacleProfile profile;			/* guarded by profile_mutex */
static bool profile_ready;			/* guarded by profile_mutex */
/* Mission -> Obstacle requests: one writer per counter (no atomic RMW on M0+). */
static _Atomic uint32_t scan_requests, scan_served, cancel_requests, cancel_served;
static _Atomic uint8_t requested_scan_id;
static rd_word_t side_command;			/* writer: Mission; bit 0 on, bits 8-23 angle */

uint32_t s5_front_word(void)
{
    return rd_word_read(&front_word);
}

uint32_t s5_side_word(void) { return rd_word_read(&side_word); }
uint32_t s5_scan_word(void) { return rd_word_read(&scan_word); }

void s5_scan_request(uint8_t scan_id)
{
    atomic_store_explicit(&requested_scan_id, scan_id & 0xFu, memory_order_relaxed);
    atomic_store_explicit(&scan_requests, atomic_load_explicit(&scan_requests, memory_order_relaxed) + 1u,
        memory_order_release);
}

void s5_scan_cancel(void)
{
    atomic_store_explicit(&cancel_requests, atomic_load_explicit(&cancel_requests, memory_order_relaxed) + 1u,
        memory_order_release);
}

void s5_side_watch(bool on, int16_t angle_deg)
{
    rd_word_publish(&side_command, (on ? 1u : 0u) | ((uint32_t)(uint16_t)angle_deg << 8));
}

ER s5_profile_copy(ObstacleProfile *out)
{
    if (profile_mutex <= 0) return E_OBJ;
    ER er = tk_loc_mtx(profile_mutex, RD_SNAPSHOT_LOCK_MS);
    if (er < E_OK) return er;
    bool ready = profile_ready;
    if (ready) *out = profile;
    ER unlock = tk_unl_mtx(profile_mutex);
    return unlock < E_OK ? unlock : (ready ? E_OK : E_OBJ);
}

static void echo_edge(unsigned pin, uint32_t edges, uint32_t sampled_us)
{
    (void)pin;
    unsigned state = atomic_load_explicit(&echo_state, memory_order_relaxed);
    if (edges == GPIO_IRQ_EDGE_RISE && state == ECHO_ARMED)
    {
        rise_us = sampled_us;
        atomic_store_explicit(&echo_state, ECHO_HIGH, memory_order_release);
    }
    else if (edges == GPIO_IRQ_EDGE_FALL && state == ECHO_HIGH)
    {
        atomic_store_explicit(&pulse_us, robot_time_elapsed(sampled_us, rise_us), memory_order_relaxed);
        atomic_store_explicit(&echo_state, ECHO_DONE, memory_order_release);
    }
    /* Any other combination (both flags, stray edge) leaves the measurement to time out. */
}

/* Servo on GP12, PWM slice 6 channel A, 50 Hz, 1 us per tick. Left disabled
   until a scan or side-watch needs it. */
static ER servo_init(void)
{
    clr_w(RESETS_RESET, RESETS_RESET_PWM);	/* release only; never reset a shared slice */
    out_w(PWM_CH6_BASE + PWM_CHx_CSR, 0u);
    out_w(PWM_CH6_BASE + PWM_CHx_CTR, 0u);
    out_w(PWM_CH6_BASE + PWM_CHx_DIV, 125u * 16u);	/* 125 MHz / 125 = 1 MHz */
    ER error = pwm_set_wrap(SERVO_PIN, 19999u);	/* 20 ms frame */
    if (error >= E_OK) error = pwm_set_cc(SERVO_PIN, 0u);
    if (error >= E_OK) error = pwm_set_pin(SERVO_PIN);
    return error;
}

static ER servo_angle(int angle_deg)
{
    if (angle_deg < -SERVO_MAX_DEG) angle_deg = -SERVO_MAX_DEG;
    if (angle_deg > SERVO_MAX_DEG) angle_deg = SERVO_MAX_DEG;
    ER error = pwm_set_cc(SERVO_PIN, (UW)((int)SERVO_CENTRE_US + angle_deg * (int)SERVO_US_PER_DEG));
    return error < E_OK ? error : pwm_set_enabled(SERVO_PIN, TRUE);
}

ER subsystem5_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    profile_mutex = tk_cre_mtx(&mutex);
    if (profile_mutex < E_OK) return profile_mutex;
    ER error = servo_init();
    if (error >= E_OK) error = gpio_set_val(TRIGGER_PIN, 0);
    if (error >= E_OK)
    {
        error = gpio_set_pin(TRIGGER_PIN, GPIO_MODE_OUT);
    }
    if (error >= E_OK)
    {
        error = gpio_set_pin(ECHO_PIN, GPIO_MODE_IN);
    }
    return error;
}

/* 10 us trigger pulse from the hardware TIMER; the only busy wait in S5. */
static ER trigger(void)
{
    ER error = gpio_set_val(TRIGGER_PIN, 0);
    if (error < E_OK) return error;
    uint32_t t = robot_time_us();
    while (robot_time_elapsed(robot_time_us(), t) < 2u) { }
    error = gpio_set_val(TRIGGER_PIN, 1);
    if (error < E_OK) return error;
    t = robot_time_us();
    while (robot_time_elapsed(robot_time_us(), t) < 10u) { }
    return gpio_set_val(TRIGGER_PIN, 0);
}

/* One ranging attempt: arm, trigger, poll the capture state per tick until the
   echo or the timeout. Returns the range in mm or ECHO_RANGE_INVALID. */
static uint32_t measure(void)
{
    (void)drv_gpio_irq_enable(ECHO_PIN, false);
    atomic_store_explicit(&echo_state, ECHO_ARMED, memory_order_release);
    (void)drv_gpio_irq_enable(ECHO_PIN, true);
    uint32_t sent_us = robot_time_us();
    if (trigger() >= E_OK)
    {
        while (atomic_load_explicit(&echo_state, memory_order_acquire) != ECHO_DONE &&
               robot_time_elapsed(robot_time_us(), sent_us) < SONAR_TIMEOUT_US)
        {
            tk_dly_tsk(1);
        }
    }
    (void)drv_gpio_irq_enable(ECHO_PIN, false);
    bool done = atomic_load_explicit(&echo_state, memory_order_acquire) == ECHO_DONE;
    uint32_t range = done ? echo_range_mm(atomic_load_explicit(&pulse_us, memory_order_relaxed)) : ECHO_RANGE_INVALID;
    atomic_store_explicit(&echo_state, ECHO_IDLE, memory_order_release);
    return range;
}

static void publish_scan(s5_scan_state_t state, uint8_t id)
{
    uint32_t word = 0u;
    (void)s5_scan_state_pack(state, &word);
    rd_word_publish(&scan_word, word | ((uint32_t)id << S5_SCAN_STATE_BITS));
}

static bool stationary_since(uint32_t left, uint32_t right)
{
    return drv_encoder_count(ENCODER_LEFT) == left && drv_encoder_count(ENCODER_RIGHT) == right;
}

/* Coarse then fine scan. Aborts (FAILED / CANCELLED) when the car moves, on
   cancel or at the deadline. Runs inside the Obstacle task. */
static void run_scan(uint8_t id)
{
    ObstacleRay rays[S5_SCAN_CAPACITY];
    unsigned count = 0u;
    uint32_t left = drv_encoder_count(ENCODER_LEFT), right = drv_encoder_count(ENCODER_RIGHT);
    uint32_t started = robot_time_ms();
    s5_scan_state_t state = S5_SCAN_SETTLING;
    publish_scan(state, id);
    int16_t angles[S5_SCAN_CAPACITY];
    unsigned planned = 0u;
    for (int a = -SERVO_MAX_DEG; a <= SERVO_MAX_DEG; a += S5_COARSE_STEP_DEG) angles[planned++] = (int16_t)a;
    unsigned coarse = planned;
    for (unsigned i = 0u; i < planned && state != S5_SCAN_FAILED && state != S5_SCAN_CANCELLED; ++i)
    {
        bool cancel = atomic_load_explicit(&cancel_requests, memory_order_acquire) !=
                      atomic_load_explicit(&cancel_served, memory_order_relaxed);
        bool expired = robot_time_elapsed(robot_time_ms(), started) >= S5_SCAN_DEADLINE_MS;
        s5_scan_state_t guard = s5_scan_guard(cancel, expired, stationary_since(left, right));
        if (guard != S5_SCAN_COARSE)
        {
            state = guard;
            break;
        }
        state = i < coarse ? S5_SCAN_COARSE : S5_SCAN_FINE;
        publish_scan(state, id);
        if (servo_angle(angles[i]) < E_OK) { state = S5_SCAN_FAILED; break; }
        tk_dly_tsk(SERVO_SETTLE_MS);
        uint32_t range = measure();
        rays[count++] = (ObstacleRay){angles[i], (uint16_t)(range == ECHO_RANGE_INVALID ? 0u : range),
                                      range != ECHO_RANGE_INVALID};
        if (i + 1u == coarse)
        {
            planned += s5_fine_angles(rays, count, &angles[planned], S5_SCAN_CAPACITY - planned);
        }
    }
    (void)servo_angle(0);
    if (state != S5_SCAN_FAILED && state != S5_SCAN_CANCELLED)
    {
        ObstacleProfile built;
        /* Missing rays (no echo) make the profile unknown: never assume clear space. */
        bool ok = s5_profile_build(rays, count, &built);
        if (ok && tk_loc_mtx(profile_mutex, RD_SNAPSHOT_LOCK_MS) >= E_OK)
        {
            profile = built;
            profile_ready = true;
            tk_unl_mtx(profile_mutex);
        }
        state = ok ? S5_SCAN_DONE : S5_SCAN_FAILED;
    }
    publish_scan(state, id);
    atomic_store_explicit(&cancel_served, atomic_load_explicit(&cancel_requests, memory_order_acquire),
        memory_order_release);
}

static void obstacle_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    bool ready = drv_gpio_irq_init() >= E_OK && drv_gpio_irq_claim(ECHO_PIN, echo_edge);
    if (!ready)
    {
        rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        tm_printf((UB *)"[S5] echo dispatcher unavailable; front range unknown\n");
    }
    uint32_t release = robot_time_ms(), misses = 0u;
    bool side_active = false;
    for (;;)
    {
        if (ready)
        {
            uint32_t requested = atomic_load_explicit(&scan_requests, memory_order_acquire);
            uint32_t command = rd_word_read(&side_command);
            if (requested != atomic_load_explicit(&scan_served, memory_order_relaxed))
            {
                atomic_store_explicit(&scan_served, requested, memory_order_release);
                run_scan(atomic_load_explicit(&requested_scan_id, memory_order_relaxed));
            }
            else if ((command & 1u) != 0u)
            {
                (void)servo_angle((int16_t)(command >> 8));
                side_active = true;
                uint32_t range = measure();
                rd_word_publish(&side_word, echo_pack(range, robot_time_ms()));
            }
            else
            {
                if (side_active)
                {
                    (void)servo_angle(0);	/* recentre, then rest the servo */
                    side_active = false;
                }
                uint32_t range = measure();
                rd_word_publish(&front_word, echo_pack(range, robot_time_ms()));
                if (echo_note(&misses, range != ECHO_RANGE_INVALID))
                {
                    rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
                }
                else
                {
                    rd_fault_clear(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
                }
            }
        }
        tk_dly_tsk(robot_time_next_delay(&release, SONAR_REPEAT_MS, robot_time_ms()));
    }
}

ER subsystem5_start(void)
{
    /* Boot never waits for an echo: ranging runs in its own periodic task. */
    ID tid = app_start_task((FP)obstacle_task, S5_PRI_OBSTACLE, TP_PRC2, "Obstacle");
    return tid < E_OK ? tid : E_OK;
}
