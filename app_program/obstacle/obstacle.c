#include "../app.h"
#include "obstacle.h"
#include <stdatomic.h>
#include "../common/drv_gpio_irq.h"
#include "../common/robot_time.h"
#include "../common/robot_config.h"
#include "../motion/motion.h"

/* Echo capture state. The handler is the only writer once the task arms the
   measurement with the Echo IRQ disabled, so no field has two writers at once. */
enum { ECHO_IDLE, ECHO_ARMED, ECHO_HIGH, ECHO_DONE };
static _Atomic unsigned echo_state;
static _Atomic uint32_t pulse_us;
static uint32_t rise_us;			/* handler-private */
static rd_word_t scan_word;			/* writer: the obstacle task */
static ID profile_mutex;
static obstacle_profile_evidence_t profile_evidence;
static uint32_t profile_sampled_ms;
static obstacle_range_sample_t report_samples[2]; /* guarded by profile_mutex */
static obstacle_range_sample_t range_samples[2]; /* Mission-only, not cross-core */
static const obstacle_profile_config_t profile_config = {
    .depth_enter_mm = OBSTACLE_PROFILE_DEPTH_ENTER_MM,
    .depth_exit_mm = OBSTACLE_PROFILE_DEPTH_EXIT_MM,
    .background_separation_mm = 0u, /* no empty-scene reference supplied */
    .range_uncertainty_mm = OBSTACLE_PROFILE_RANGE_ERROR_MM,
    .planning_width_mm = OBSTACLE_EXPECTED_MAX_WIDTH_MM,
    .adjacent_rays = OBSTACLE_PROFILE_ADJACENT_RAYS,
    .maximum_gap_deg = OBSTACLE_COARSE_STEP_DEG};
static ObstacleProfile profile;			/* guarded by profile_mutex */
static bool profile_ready;			/* guarded by profile_mutex */
/* Mission -> Obstacle requests: one writer per counter (no atomic RMW on M0+). */
static _Atomic uint32_t scan_requests, scan_served, cancel_requests, cancel_served;
static _Atomic uint8_t requested_scan_id;
static rd_word_t side_command;			/* writer: Mission; bit 0 on, bits 1-8 signed servo angle */

uint32_t obstacle_scan_word(void) { return rd_word_read(&scan_word); }

void obstacle_scan_request(uint8_t scan_id)
{
    if (scan_id > 15u)
    {
        return;
    }
    atomic_store_explicit(&requested_scan_id, scan_id, memory_order_relaxed);
    atomic_store_explicit(&scan_requests, atomic_load_explicit(&scan_requests, memory_order_relaxed) + 1u,
        memory_order_release);
}

void obstacle_scan_cancel(void)
{
    atomic_store_explicit(&cancel_requests, atomic_load_explicit(&cancel_requests, memory_order_relaxed) + 1u,
        memory_order_release);
}

void obstacle_side_watch(bool on, int16_t angle_deg)
{
    rd_word_publish(&side_command, obstacle_side_watch_pack(on, angle_deg));
}

static ER profile_copy(ObstacleProfile *out, TMO wait)
{
    if (out == NULL || profile_mutex <= 0) return E_OBJ;
    ER er = tk_loc_mtx(profile_mutex, wait);
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

ER obstacle_init(void)
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
typedef enum { OBSTACLE_ACQUIRE_IDLE, OBSTACLE_ACQUIRE_SETTLE, OBSTACLE_ACQUIRE_ECHO } obstacle_acquire_phase_t;

typedef struct
{
    obstacle_range_group_t group;
    ObstacleRay rays[OBSTACLE_SCAN_CAPACITY];
    int16_t angles[OBSTACLE_SCAN_CAPACITY];
    uint32_t scan_started_ms, sent_us, settle_since_ms, next_ping_ms;
    uint32_t left_origin, right_origin;
    uint32_t misses;
    uint16_t count : 4;
    uint16_t planned : 4;
    uint16_t coarse : 4;
    uint16_t id : 4;
    uint8_t phase : 2;
    uint8_t initialized : 1;
    uint8_t ready : 1;
    uint8_t scanning : 1;
    int8_t angle : 6;
    int8_t target_angle : 6;
} obstacle_acquisition_t;

_Static_assert(OBSTACLE_SCAN_CAPACITY <= 15u,
    "scan counts must fit their four-bit fields");
_Static_assert((2 * SERVO_MAX_DEG) / OBSTACLE_COARSE_STEP_DEG + 1 <= OBSTACLE_SCAN_CAPACITY,
    "coarse scan must fit fixed capture storage");
_Static_assert(SERVO_MAX_DEG % OBSTACLE_COARSE_STEP_DEG == 0,
    "scan spacing must include centre and both endpoints");
_Static_assert(OBSTACLE_SCAN_GROUP_SAMPLES == 0u ||
    (OBSTACLE_SCAN_GROUP_SAMPLES >= 3u && OBSTACLE_SCAN_GROUP_SAMPLES <= OBSTACLE_RANGE_GROUP_CAPACITY && (OBSTACLE_SCAN_GROUP_SAMPLES & 1u) != 0u),
    "scan group is unknown or an odd bounded count");
_Static_assert(OBSTACLE_SCAN_MIN_VALID <= OBSTACLE_SCAN_GROUP_SAMPLES,
    "valid sample minimum must fit its group");
_Static_assert(OBSTACLE_PROFILE_ADJACENT_RAYS == 0u ||
    (OBSTACLE_PROFILE_ADJACENT_RAYS >= 2u && OBSTACLE_PROFILE_ADJACENT_RAYS <= OBSTACLE_SCAN_CAPACITY),
    "edge qualification needs bounded adjacent rays");
_Static_assert(OBSTACLE_PROFILE_DEPTH_ENTER_MM <= 65535u &&
    OBSTACLE_PROFILE_DEPTH_EXIT_MM <= 65535u && OBSTACLE_PROFILE_RANGE_ERROR_MM <= 65535u,
    "profile settings must fit their representation");
static obstacle_acquisition_t acquisition;

static void publish_scan(obstacle_scan_state_t state, uint8_t id)
{
    uint32_t word = 0u;
    (void)obstacle_scan_state_pack(state, &word);
    rd_word_publish(&scan_word, word | obstacle_scan_id_pack(id));
}

static void cancel_echo(void)
{
    (void)drv_gpio_irq_enable(ECHO_PIN, false);
    atomic_store_explicit(&echo_state, ECHO_IDLE, memory_order_release);
}

static bool set_angle(int16_t angle, uint32_t now)
{
    if (angle < -SERVO_MAX_DEG || angle > SERVO_MAX_DEG)
    {
        return false;
    }
    cancel_echo();
    acquisition.target_angle = angle;
    acquisition.settle_since_ms = now;
    acquisition.phase = OBSTACLE_ACQUIRE_SETTLE;
    /* Keep the current command; obstacle_step slews toward the new target. */
    return servo_angle(acquisition.angle) >= E_OK;
}

static void finish_scan(obstacle_scan_state_t state, uint32_t now)
{
    if (state == OBSTACLE_SCAN_DONE)
    {
        ObstacleProfile built;
        obstacle_profile_evidence_t evidence;
        bool ok = obstacle_profile_estimate(acquisition.rays, acquisition.count, NULL,
            &profile_config, &built, &evidence);
        if (ok && tk_loc_mtx(profile_mutex, TMO_POL) >= E_OK)
        {
            profile = built;
            profile_evidence = evidence;
            profile_sampled_ms = now;
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
        state = ok ? OBSTACLE_SCAN_DONE : OBSTACLE_SCAN_FAILED;
    }
    acquisition.scanning = false;
    cancel_echo();
    publish_scan(state, acquisition.id);
    (void)set_angle(0, now);
}

/* Publish settled scan groups as they arrive, with their real direction. */
static void publish_range(uint32_t range, uint32_t now)
{
    if (acquisition.angle != 0)
    {
        uint32_t word = echo_pack(range, now);
        range_samples[1].word = word;
        range_samples[1].sampled_ms = now;
        ++range_samples[1].sequence;
    }
    else
    {
        uint32_t word = echo_pack(range, now);
        range_samples[0].word = word;
        range_samples[0].sampled_ms = now;
        range_samples[0].wheel_count[0] = drv_encoder_count(ENCODER_LEFT);
        range_samples[0].wheel_count[1] = drv_encoder_count(ENCODER_RIGHT);
        ++range_samples[0].sequence;
        if (echo_note(&acquisition.misses, range != ECHO_RANGE_INVALID))
        {
            rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        }
        else
        {
            rd_fault_clear(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
        }
    }
    if (tk_loc_mtx(profile_mutex, TMO_POL) >= E_OK)
    {
        report_samples[0] = range_samples[0];
        report_samples[1] = range_samples[1];
        (void)tk_unl_mtx(profile_mutex);
    }
}

static void range_complete(uint32_t range, uint32_t now)
{
    cancel_echo();
    acquisition.phase = OBSTACLE_ACQUIRE_IDLE;
    if (acquisition.scanning)
    {
        if (acquisition.group.requested == 0u &&
            !obstacle_range_group_begin(&acquisition.group, OBSTACLE_SCAN_GROUP_SAMPLES,
                OBSTACLE_SCAN_MIN_VALID))
        {
            finish_scan(OBSTACLE_SCAN_FAILED, now);
            return;
        }
        if (!obstacle_range_group_push(&acquisition.group, range))
        {
            finish_scan(OBSTACLE_SCAN_FAILED, now);
            return;
        }
        if (!obstacle_range_group_complete(&acquisition.group))
        {
            return; /* Same settled angle, next trigger at the repeat deadline. */
        }
        uint16_t filtered;
        range = obstacle_range_group_median(&acquisition.group, &filtered) ?
            filtered : ECHO_RANGE_INVALID;
        acquisition.group = (obstacle_range_group_t){0};
        publish_range(range, now);
        unsigned index = acquisition.count;
        acquisition.rays[index] = obstacle_ray_make(acquisition.angles[index],
            (uint16_t)(range == ECHO_RANGE_INVALID ? 0u : range),
            range != ECHO_RANGE_INVALID);
        ++acquisition.count;
        if (acquisition.count == acquisition.coarse)
        {
            unsigned extra = obstacle_profile_fine_angles(acquisition.rays,
                acquisition.count, &profile_config, &acquisition.angles[acquisition.planned],
                OBSTACLE_SCAN_CAPACITY - acquisition.planned);
            acquisition.planned = (uint8_t)(acquisition.planned + extra);
        }
        if (acquisition.count == acquisition.planned)
        {
            finish_scan(OBSTACLE_SCAN_DONE, now);
        }
        else
        {
            publish_scan(acquisition.count < acquisition.coarse ?
                OBSTACLE_SCAN_COARSE : OBSTACLE_SCAN_FINE, acquisition.id);
            if (!set_angle(acquisition.angles[acquisition.count], now))
            {
                finish_scan(OBSTACLE_SCAN_FAILED, now);
            }
        }
        return;
    }
    publish_range(range, now);
}

void obstacle_step(uint32_t now)
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
    if (!acquisition.ready || acquisition.phase > OBSTACLE_ACQUIRE_ECHO)
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
            finish_scan(OBSTACLE_SCAN_CANCELLED, now);
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
        obstacle_scan_state_t guard = obstacle_scan_guard(false,
            robot_time_elapsed(now, acquisition.scan_started_ms) >= OBSTACLE_SCAN_DEADLINE_MS,
            stationary);
        if (guard != OBSTACLE_SCAN_COARSE)
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
        cancel_echo();
        acquisition.phase = OBSTACLE_ACQUIRE_IDLE;
        acquisition.scan_started_ms = now;
        acquisition.group = (obstacle_range_group_t){0};
        if (OBSTACLE_SCAN_GROUP_SAMPLES == 0u || OBSTACLE_SCAN_MIN_VALID == 0u ||
            OBSTACLE_PROFILE_ADJACENT_RAYS < 2u || OBSTACLE_PROFILE_DEPTH_ENTER_MM == 0u ||
            OBSTACLE_PROFILE_DEPTH_EXIT_MM < OBSTACLE_PROFILE_DEPTH_ENTER_MM ||
            !obstacle_range_group_begin(&acquisition.group, OBSTACLE_SCAN_GROUP_SAMPLES,
                OBSTACLE_SCAN_MIN_VALID))
        {
            publish_scan(OBSTACLE_SCAN_FAILED, acquisition.id);
            return; /* Explicit pending D05, never execute an unsupported scan. */
        }
        acquisition.count = 0u;
        acquisition.planned = 0u;
        for (int angle = -SERVO_MAX_DEG; angle <= SERVO_MAX_DEG;
            angle += OBSTACLE_COARSE_STEP_DEG)
        {
            acquisition.angles[acquisition.planned++] = (int16_t)angle;
        }
        acquisition.coarse = acquisition.planned;
        acquisition.scanning = true;
        publish_scan(OBSTACLE_SCAN_SETTLING, acquisition.id);
        if (!set_angle(acquisition.angles[0], now))
        {
            finish_scan(OBSTACLE_SCAN_FAILED, now);
        }
        return;
    }
    if (!acquisition.scanning)
    {
        uint32_t command = rd_word_read(&side_command);
        int16_t angle = (command & 1u) != 0u ? obstacle_side_watch_angle(command) : 0;
        if (angle != acquisition.target_angle && !set_angle(angle, now))
        {
            rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
            return;
        }
    }
    if (acquisition.phase == OBSTACLE_ACQUIRE_SETTLE)
    {
        if (acquisition.angle != acquisition.target_angle)
        {
            if (robot_time_elapsed(now, acquisition.settle_since_ms) <
                SERVO_STEP_MS)
            {
                return;
            }
            /* One step per release: a delayed release never causes a jump. */
            acquisition.angle += acquisition.angle <
                acquisition.target_angle ? 1 : -1;
            acquisition.settle_since_ms = now;
            if (servo_angle(acquisition.angle) < E_OK)
            {
                acquisition.ready = false;
                if (acquisition.scanning)
                {
                    finish_scan(OBSTACLE_SCAN_FAILED, now);
                }
                rd_fault_set(RD_OWNER_OBSTACLE, RD_FAULT_NO_ECHO);
            }
            return;
        }
        if (robot_time_elapsed(now, acquisition.settle_since_ms) < SERVO_SETTLE_MS)
        {
            return;
        }
        acquisition.phase = OBSTACLE_ACQUIRE_IDLE;
    }
    if (acquisition.phase == OBSTACLE_ACQUIRE_ECHO)
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
    acquisition.phase = OBSTACLE_ACQUIRE_ECHO;
    if (trigger() < E_OK)
    {
        range_complete(ECHO_RANGE_INVALID, now);
    }
}

ER obstacle_profile_try_copy(ObstacleProfile *out)
{
    return profile_copy(out, TMO_POL);
}

ER obstacle_range_samples(obstacle_range_sample_t *front, obstacle_range_sample_t *side)
{
    if (front == NULL || side == NULL)
    {
        return E_PAR;
    }
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
    *front = range_samples[0];
    *side = range_samples[1];
    return E_OK;
}

ER obstacle_profile_snapshot(obstacle_profile_snapshot_t *out)
{
    if (out == NULL || profile_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(profile_mutex, RD_SNAPSHOT_LOCK_MS);
    if (er < E_OK)
    {
        return er;
    }
    bool ready = profile_ready;
    if (ready)
    {
        *out = (obstacle_profile_snapshot_t){.profile = profile,
            .evidence = profile_evidence, .sampled_ms = profile_sampled_ms};
    }
    er = tk_unl_mtx(profile_mutex);
    return er < E_OK ? er : (ready ? E_OK : E_OBJ);
}

ER obstacle_range_try_copy(obstacle_range_sample_t *front, obstacle_range_sample_t *side)
{
    if (front == NULL || side == NULL || profile_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(profile_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    *front = report_samples[0];
    *side = report_samples[1];
    return tk_unl_mtx(profile_mutex);
}
