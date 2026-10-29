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
static rd_word_t side_command;			/* writer: Mission; bit 0 on, bits 1-7 signed servo angle */

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
    rd_word_publish(&side_command, s5_side_watch_pack(on, angle_deg));
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

/* Acquisition phases never sleep through an Echo or servo movement. Mission
   calls one step per release; only the 12 us trigger pulse busy-waits. */
typedef enum { S5_ACQUIRE_IDLE, S5_ACQUIRE_SETTLE, S5_ACQUIRE_ECHO } s5_acquire_phase_t;

typedef struct
{
    ObstacleRay rays[S5_SCAN_CAPACITY];
    int16_t angles[S5_SCAN_CAPACITY];
    uint32_t scan_started_ms, sent_us, settle_since_ms, next_ping_ms;
    uint32_t left_origin, right_origin;
    uint32_t misses;
    uint8_t count, planned, coarse, id;
    uint8_t phase;
    uint8_t initialized : 1;
    uint8_t ready : 1;
    uint8_t scanning : 1;
    int16_t angle;
} s5_acquisition_t;

static s5_acquisition_t acquisition;

static void publish_scan(s5_scan_state_t state, uint8_t id)
{
    uint32_t word = 0u;
    (void)s5_scan_state_pack(state, &word);
    rd_word_publish(&scan_word, word | ((uint32_t)id << S5_SCAN_STATE_BITS));
}

static void cancel_echo(void)
{
    (void)drv_gpio_irq_enable(ECHO_PIN, false);
    atomic_store_explicit(&echo_state, ECHO_IDLE, memory_order_release);
}

static bool set_angle(int16_t angle, uint32_t now)
{
    cancel_echo();
    acquisition.angle = angle;
    acquisition.settle_since_ms = now;
    acquisition.phase = S5_ACQUIRE_SETTLE;
    return servo_angle(angle) >= E_OK;
}

static void finish_scan(s5_scan_state_t state, uint32_t now)
{
    if (state == S5_SCAN_DONE)
    {
        ObstacleProfile built;
        bool ok = s5_profile_build(acquisition.rays, acquisition.count, &built);
        if (ok && tk_loc_mtx(profile_mutex, TMO_POL) >= E_OK)
        {
            profile = built;
            profile_ready = true;
            if (tk_unl_mtx(profile_mutex) < E_OK)
            {
                ok = false;
            }
        }
        else
        {
            ok = false;
        }
        state = ok ? S5_SCAN_DONE : S5_SCAN_FAILED;
    }
    acquisition.scanning = false;
    cancel_echo();
    publish_scan(state, acquisition.id);
    (void)set_angle(0, now);
}

static void range_complete(uint32_t range, uint32_t now)
{
    cancel_echo();
    acquisition.phase = S5_ACQUIRE_IDLE;
    if (acquisition.scanning)
    {
        unsigned index = acquisition.count;
        acquisition.rays[index] = s5_ray_make(acquisition.angles[index],
            (uint16_t)(range == ECHO_RANGE_INVALID ? 0u : range),
            range != ECHO_RANGE_INVALID);
        ++acquisition.count;
        if (acquisition.count == acquisition.coarse)
        {
            unsigned extra = s5_fine_angles(acquisition.rays,
                acquisition.count, &acquisition.angles[acquisition.planned],
                S5_SCAN_CAPACITY - acquisition.planned);
            acquisition.planned = (uint8_t)(acquisition.planned + extra);
        }
        if (acquisition.count == acquisition.planned)
        {
            finish_scan(S5_SCAN_DONE, now);
        }
        else
        {
            publish_scan(acquisition.count < acquisition.coarse ?
                S5_SCAN_COARSE : S5_SCAN_FINE, acquisition.id);
            if (!set_angle(acquisition.angles[acquisition.count], now))
            {
                finish_scan(S5_SCAN_FAILED, now);
            }
        }
        return;
    }
    if (acquisition.angle != 0)
    {
        rd_word_publish(&side_word, echo_pack(range, now));
    }
    else
    {
        rd_word_publish(&front_word, echo_pack(range, now));
        if (echo_note(&acquisition.misses, range != ECHO_RANGE_INVALID))
        {
            rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        }
        else
        {
            rd_fault_clear(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        }
    }
}

void s5_step(uint32_t now)
{
    if (!acquisition.initialized)
    {
        acquisition.initialized = true;
        acquisition.ready = drv_gpio_irq_init() >= E_OK &&
            drv_gpio_irq_claim(ECHO_PIN, echo_edge);
        acquisition.next_ping_ms = now;
        if (!acquisition.ready || !set_angle(0, now))
        {
            acquisition.ready = false;
            rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        }
    }
    if (!acquisition.ready)
    {
        return;
    }
    uint32_t cancel = atomic_load_explicit(&cancel_requests, memory_order_acquire);
    bool cancelled = cancel != atomic_load_explicit(&cancel_served, memory_order_relaxed);
    atomic_store_explicit(&cancel_served, cancel, memory_order_release);
    uint32_t requested = atomic_load_explicit(&scan_requests, memory_order_acquire);
    if (cancelled)
    {
        atomic_store_explicit(&scan_served, requested, memory_order_release);
        if (acquisition.scanning)
        {
            finish_scan(S5_SCAN_CANCELLED, now);
        }
        else
        {
            cancel_echo();
            (void)set_angle(0, now);
        }
        return;
    }
    if (acquisition.scanning)
    {
        bool stationary = drv_encoder_count(ENCODER_LEFT) == acquisition.left_origin &&
            drv_encoder_count(ENCODER_RIGHT) == acquisition.right_origin;
        s5_scan_state_t guard = s5_scan_guard(false,
            robot_time_elapsed(now, acquisition.scan_started_ms) >= S5_SCAN_DEADLINE_MS,
            stationary);
        if (guard != S5_SCAN_COARSE)
        {
            finish_scan(guard, now);
            return;
        }
    }
    else if (requested != atomic_load_explicit(&scan_served, memory_order_relaxed))
    {
        atomic_store_explicit(&scan_served, requested, memory_order_release);
        acquisition.id = atomic_load_explicit(&requested_scan_id, memory_order_relaxed);
        acquisition.left_origin = drv_encoder_count(ENCODER_LEFT);
        acquisition.right_origin = drv_encoder_count(ENCODER_RIGHT);
        acquisition.scan_started_ms = now;
        acquisition.count = 0u;
        acquisition.planned = 0u;
        for (int a = -SERVO_MAX_DEG; a <= SERVO_MAX_DEG; a += S5_COARSE_STEP_DEG)
        {
            acquisition.angles[acquisition.planned++] = (int16_t)a;
        }
        acquisition.coarse = acquisition.planned;
        acquisition.scanning = true;
        publish_scan(S5_SCAN_SETTLING, acquisition.id);
        if (!set_angle(acquisition.angles[0], now))
        {
            finish_scan(S5_SCAN_FAILED, now);
        }
        return;
    }
    if (!acquisition.scanning)
    {
        uint32_t command = rd_word_read(&side_command);
        int16_t angle = (command & 1u) != 0u ? s5_side_watch_angle(command) : 0;
        if (angle != acquisition.angle && !set_angle(angle, now))
        {
            rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
            return;
        }
    }
    if (acquisition.phase == S5_ACQUIRE_SETTLE)
    {
        if (robot_time_elapsed(now, acquisition.settle_since_ms) < SERVO_SETTLE_MS)
        {
            return;
        }
        acquisition.phase = S5_ACQUIRE_IDLE;
    }
    if (acquisition.phase == S5_ACQUIRE_ECHO)
    {
        if (atomic_load_explicit(&echo_state, memory_order_acquire) == ECHO_DONE)
        {
            range_complete(echo_range_mm(atomic_load_explicit(&pulse_us,
                memory_order_relaxed)), now);
        }
        else if (robot_time_elapsed(robot_time_us(), acquisition.sent_us) >= SONAR_TIMEOUT_US)
        {
            range_complete(ECHO_RANGE_INVALID, now);
        }
        return;
    }
    if ((int32_t)(now - acquisition.next_ping_ms) < 0)
    {
        return;
    }
    cancel_echo();
    atomic_store_explicit(&echo_state, ECHO_ARMED, memory_order_release);
    (void)drv_gpio_irq_enable(ECHO_PIN, true);
    acquisition.sent_us = robot_time_us();
    acquisition.next_ping_ms = now + SONAR_REPEAT_MS;
    acquisition.phase = S5_ACQUIRE_ECHO;
    if (trigger() < E_OK)
    {
        range_complete(ECHO_RANGE_INVALID, now);
    }
}

ER subsystem5_start(void)
{
    return E_OK; /* Mission owns acquisition; no Obstacle task. */
}
