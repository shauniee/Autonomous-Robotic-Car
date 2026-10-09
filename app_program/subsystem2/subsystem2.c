#include "subsystem2.h"
#include "drv_motor.h"
#include "../gpio_capture.h"
#include <tk/smp.h>
#include <stdatomic.h>

#define MOTION_PERIOD_MS 20u
#define REQUEST_WAIT_MS 2
#define STOP_CAS_ATTEMPTS 8u

static atomic_int request_mutex;
static atomic_int stop_flag;
static atomic_int motion_tid;
static UW stop_state = MOTION_BOOT_STATE;
static MotionRequest latest;
static MotionHealth progress;
static uint32_t pending_reset;
static bool driver_ready;
static S2NavigationConfig navigation_config;

bool s2_navigation_configure(const S2NavigationConfig *config)
{
    if (!s2_navigation_config_valid(config) ||
        (atomic_load_explicit(&motion_tid, memory_order_acquire) > 0))
    {
        return false;
    }
    navigation_config = *config;
    return true;
}

bool s2_navigation_configuration(S2NavigationConfig *config)
{
    if (config == NULL)
    {
        return false;
    }
    *config = navigation_config;
    return s2_navigation_config_valid(config);
}

/* M0+ C11 compare-exchange needs an unavailable libatomic helper. Use the
   public SMP kernel RMW primitive; local interrupt masking is only for UP. */
static uint32_t state_read(void)
{
#if TK_SUPPORT_SMP
    return (uint32_t)atomic_add(&stop_state, 0u);
#else
    UINT saved;
    DI(saved);
    uint32_t state = stop_state;
    EI(saved);
    return state;
#endif
}

static uint32_t state_compare_exchange(uint32_t expected, uint32_t desired)
{
#if TK_SUPPORT_SMP
    return (uint32_t)atomic_cmpxchg(&stop_state, desired, expected);
#else
    UINT saved;
    DI(saved);
    uint32_t previous = stop_state;
    if (previous == expected)
    {
        stop_state = desired;
    }
    EI(saved);
    return previous;
#endif
}

static void state_inhibit_permanently(void)
{
#if TK_SUPPORT_SMP
    (void)atomic_xchg(&stop_state, UINT32_MAX);
#else
    UINT saved;
    DI(saved);
    stop_state = UINT32_MAX;
    EI(saved);
#endif
}

static ER advance_stop(void)
{
    uint32_t previous = state_read();
    for (unsigned attempt = 0u; attempt < STOP_CAS_ATTEMPTS; ++attempt)
    {
        uint32_t next = motion_stop_next(previous);
        uint32_t observed = state_compare_exchange(previous, next);
        if (observed == previous)
        {
            return next == UINT32_MAX ? E_SYS : E_OK;
        }
        previous = observed;
    }
    /* Never spin indefinitely or allow generation reuse after contention. */
    state_inhibit_permanently();
    return E_BUSY;
}

static bool read_time(uint32_t *now)
{
    SYSTIM time;
    if (tk_get_otm(&time) < E_OK)
    {
        return false;
    }
    *now = time.lo;
    return true;
}

static void set_reason(MotionHealth *health, const char *reason)
{
    unsigned index = 0u;
    while ((index < sizeof(health->reason) - 1u) && (reason[index] != '\0'))
    {
        health->reason[index] = reason[index];
        ++index;
    }
    health->reason[index] = '\0';
}

static ER wake_motion(void)
{
    ID tid = atomic_load_explicit(&motion_tid, memory_order_acquire);
    return tid > 0 ? tk_wup_tsk(tid) : E_OK;
}

ER s2_stop(void)
{
    ID flag = atomic_load_explicit(&stop_flag, memory_order_acquire);
    if (flag <= 0)
    {
        return E_OBJ;
    }
    ER state_error = advance_stop();
    ER flag_error = tk_set_flg(flag, MOTION_STOP_BIT);
    ER wake_error = wake_motion();
    if (state_error < E_OK)
    {
        return state_error;
    }
    if (flag_error < E_OK)
    {
        return flag_error;
    }
    return wake_error;
}

ER s2_publish(MotionRequest request)
{
    ID mutex = atomic_load_explicit(&request_mutex, memory_order_acquire);
    if (mutex <= 0)
    {
        return E_OBJ;
    }
    if (request.enabled &&
        ((request.velocity_mode ?
          !s2_navigation_config_valid(&navigation_config) :
          !motion_duties_valid(request.left, request.right)) ||
         (request.generation == 0u) ||
         (request.generation >= MOTION_MAX_GENERATION)))
    {
        (void)s2_stop();
        return E_PAR;
    }
    ER error = tk_loc_mtx(mutex, REQUEST_WAIT_MS);
    if (error < E_OK)
    {
        (void)s2_stop();
        return error;
    }
    /* Gate under the mailbox mutex: an enabled target published between
       reset's discard and its CAS must not survive into the armed state.
       Even after the CAS, publication waits for Motion's reset snapshot. */
    if (!motion_publication_allowed(request, state_read(),
        progress.reset_ack_generation, progress.driver_ok))
    {
        ER unlock_error = tk_unl_mtx(mutex);
        (void)s2_stop();
        return unlock_error < E_OK ? unlock_error : E_OBJ;
    }
    latest = request;
    error = tk_unl_mtx(mutex);
    if (error < E_OK)
    {
        (void)s2_stop();
    }
    return error;
}

ER s2_request_reset(uint32_t expected_generation)
{
    if ((expected_generation == 0u) ||
        (expected_generation >= MOTION_MAX_GENERATION))
    {
        return E_PAR;
    }
    ID mutex = atomic_load_explicit(&request_mutex, memory_order_acquire);
    if (mutex <= 0)
    {
        return E_OBJ;
    }
    uint32_t state = state_read();
    if (!motion_stop_latched(state) ||
        (motion_stop_generation(state) != expected_generation))
    {
        return E_OBJ;
    }
    ER error = tk_loc_mtx(mutex, REQUEST_WAIT_MS);
    if (error < E_OK)
    {
        return error;
    }
    pending_reset = expected_generation;
    error = tk_unl_mtx(mutex);
    if (error < E_OK)
    {
        (void)s2_stop();
        return error;
    }
    return wake_motion();
}

ER s2_health(MotionHealth *health)
{
    if (health == NULL)
    {
        return E_PAR;
    }
    ID mutex = atomic_load_explicit(&request_mutex, memory_order_acquire);
    if (mutex <= 0)
    {
        return E_OBJ;
    }
    ER error = tk_loc_mtx(mutex, TMO_POL);
    if (error < E_OK)
    {
        return error;
    }
    MotionHealth snapshot = progress;
    error = tk_unl_mtx(mutex);
    if (error < E_OK)
    {
        return error;
    }
    /* A stop arriving while the mutex was held must still appear live.
       Older brake/reset acknowledgements remain tagged with their old token. */
    uint32_t state = state_read();
    uint32_t generation = motion_stop_generation(state);
    if (snapshot.stop_generation != generation)
    {
        set_reason(&snapshot, "stop_pending");
    }
    snapshot.stop_generation = generation;
    snapshot.stopped = motion_stop_latched(state);
    *health = snapshot;
    return E_OK;
}

bool s2_motion_copy(MotionHealth *health)
{
    return s2_health(health) >= E_OK;
}

static ER take_requests(MotionRequest *request, uint32_t *reset)
{
    ID mutex = atomic_load_explicit(&request_mutex, memory_order_acquire);
    ER error = tk_loc_mtx(mutex, REQUEST_WAIT_MS);
    if (error < E_OK)
    {
        return error;
    }
    *reset = pending_reset;
    pending_reset = 0u;
    if (*reset != 0u)
    {
        /* A reset attempt cannot leave a pre-reset target waiting to replay. */
        latest = (MotionRequest){0};
    }
    *request = latest;
    return tk_unl_mtx(mutex);
}

static ER publish_progress(const MotionHealth *health)
{
    ID mutex = atomic_load_explicit(&request_mutex, memory_order_acquire);
    ER error = tk_loc_mtx(mutex, REQUEST_WAIT_MS);
    if (error < E_OK)
    {
        return error;
    }
    progress = *health;
    return tk_unl_mtx(mutex);
}

static void latch_fault(const char *reason, const char **latched_reason)
{
    if (!motion_stop_latched(state_read()))
    {
        (void)advance_stop();
    }
    *latched_reason = reason;
}

static void park_fault(const char *reason, MotionHealth *health)
{
    (void)advance_stop();
    bool braked = drv_motor_brake();
    health->output_braked = braked;
    health->driver_ok = false;
    health->stopped = true;
    health->stop_generation = motion_stop_generation(state_read());
    health->brake_ack_generation = 0u;
    if (braked)
    {
        health->applied_left = 0.0f;
        health->applied_right = 0.0f;
    }
    set_reason(health, reason);
    (void)publish_progress(health);
    for (;;)
    {
        (void)tk_slp_tsk(TMO_FEVR);
    }
}

static void motion_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    MotionHealth health = {
        .stop_generation = 1u,
        .stopped = true,
        .test_inhibited = APP_TEST != 0,
        .driver_ok = driver_ready
    };
    uint32_t now;
    if (!read_time(&now))
    {
        park_fault("clock_failed", &health);
    }
    S2NavigationController navigation;
    s2_navigation_reset(&navigation);
    uint32_t next = now + MOTION_PERIOD_MS;
    uint32_t observed_generation = 1u;
    const char *reason = "boot_stopped";
    for (;;)
    {
        if (!read_time(&now))
        {
            park_fault("clock_failed", &health);
        }
        uint32_t state = state_read();
        uint32_t generation = motion_stop_generation(state);
        if (generation != observed_generation)
        {
            reason = "stop_requested";
        }
        MotionRequest request = {0};
        uint32_t reset = 0u;
        ER request_error = take_requests(&request, &reset);
        if (request_error < E_OK)
        {
            latch_fault("request_mutex_failed", &reason);
        }
        if ((reset != 0u) && (request_error >= E_OK))
        {
            state = state_read();
            bool allowed = motion_reset_allowed(state, reset,
                health.brake_ack_generation, health.output_braked,
                driver_ready);
            if (allowed && drv_motor_brake())
            {
                /* CAS cannot clear a newer independent stop. Only this task
                   acknowledges reset, after discarding the previous target. */
                uint32_t armed = state & ~MOTION_STOP_BIT;
                if (state_compare_exchange(state, armed) == state)
                {
                    health.reset_ack_generation = reset;
                    reason = "ready";
                    ID flag = atomic_load_explicit(&stop_flag,
                        memory_order_acquire);
                    /* Flag is a notification only; atomic state is authority.
                       A racing flag clear cannot erase the newer stop token. */
                    if (tk_clr_flg(flag, 0u) < E_OK)
                    {
                        latch_fault("reset_flag_failed", &reason);
                    }
                }
                else
                {
                    reason = "reset_superseded";
                }
            }
            else
            {
                reason = "reset_not_acknowledged";
            }
        }
        if (!read_time(&now))
        {
            park_fault("clock_failed", &health);
        }
        state = state_read();
        if (state == UINT32_MAX)
        {
            reason = "generation_exhausted";
        }
        bool apply = (request_error >= E_OK) && driver_ready &&
            motion_request_current(request, state, now);
        if (request.enabled && !motion_stop_latched(state) && !apply)
        {
            latch_fault("invalid_or_expired_request", &reason);
        }
        S2EncoderSnapshot encoders = {0};
        if (s2_encoder_copy(&encoders))
        {
            health.encoders = encoders;
        }
        else
        {
            health.encoders.valid = false;
        }
        float output_left = request.left;
        float output_right = request.right;
        if (apply && request.velocity_mode)
        {
            if (!s2_navigation_tick(&navigation, &navigation_config,
                    &encoders, &request.navigation, now,
                    &output_left, &output_right))
            {
                apply = false;
                latch_fault(navigation.feedback.reason, &reason);
            }
        }
        else if (s2_navigation_config_valid(&navigation_config))
        {
            if ((generation != observed_generation) || (reset != 0u))
            {
                s2_navigation_reset(&navigation);
            }
            S2NavigationRequest idle = {.kind = S2_SPEED};
            float unused_left;
            float unused_right;
            (void)s2_navigation_tick(&navigation, &navigation_config,
                &encoders, &idle, now, &unused_left, &unused_right);
        }
        health.navigation = navigation.feedback;
        /* Close the copy/validation window. A stop may still arrive during
           the bounded driver transaction; its post-check then brakes. */
        uint32_t before_output = state_read();
        apply = apply && (before_output == state);
        bool output_ok;
#if APP_TEST
        (void)apply;
        output_ok = drv_motor_brake();
#else
        output_ok = apply ?
            drv_motor_apply_signed(output_left, output_right) : drv_motor_brake();
#endif
        uint32_t after_output = state_read();
        if ((after_output != before_output) ||
            motion_stop_latched(after_output))
        {
            before_output = after_output;
            output_ok = drv_motor_brake() && output_ok;
            after_output = state_read();
        }
        if (!output_ok)
        {
            driver_ready = false;
            latch_fault("motor_driver_failed", &reason);
        }
        float left = health.applied_left;
        float right = health.applied_right;
        if (!drv_motor_output_copy(&left, &right))
        {
            driver_ready = false;
            latch_fault("motor_readback_failed", &reason);
            (void)drv_motor_brake();
            output_ok = false;
        }
        health.applied_left = left;
        health.applied_right = right;
        health.output_braked = output_ok && (left == 0.0f) &&
            (right == 0.0f);
        health.driver_ok = driver_ready && output_ok;
        state = state_read();
        health.stop_generation = motion_stop_generation(state);
        health.stopped = motion_stop_latched(state);
        if (health.output_braked && health.stopped &&
            (before_output == after_output) && (state == after_output))
        {
            health.brake_ack_generation = health.stop_generation;
        }
        if (!read_time(&now))
        {
            park_fault("clock_failed", &health);
        }
        if (health.driver_ok)
        {
            health.serviced_ms = now; /* Successful output/readback completion. */
            ++health.sequence;
        }
        set_reason(&health, reason);
        if (publish_progress(&health) < E_OK)
        {
            latch_fault("health_mutex_failed", &reason);
            (void)drv_motor_brake();
        }
        observed_generation = motion_stop_generation(state_read());
        if (!read_time(&now))
        {
            park_fault("clock_failed", &health);
        }
        if ((int32_t)(now - next) >= 0)
        {
            next += MOTION_PERIOD_MS;
            if ((int32_t)(now - next) >= 0)
            {
                next = now + MOTION_PERIOD_MS;
            }
        }
        ER delay_error = tk_slp_tsk((TMO)(next - now));
        if ((delay_error != E_OK) && (delay_error != E_TMOUT))
        {
            park_fault("motion_wait_failed", &health);
        }
    }
}

ER subsystem2_init(void)
{
    if (atomic_load_explicit(&request_mutex, memory_order_acquire) > 0)
    {
        return E_OBJ;
    }
    if (!s2_encoder_init() || !drv_motor_init())
    {
        return E_SYS;
    }
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID mutex_id = tk_cre_mtx(&mutex);
    if (mutex_id <= 0)
    {
        return mutex_id < E_OK ? mutex_id : E_SYS;
    }
    T_CFLG flag = {0};
    flag.iflgptn = MOTION_STOP_BIT;
    ID flag_id = tk_cre_flg(&flag);
    if (flag_id <= 0)
    {
        (void)tk_del_mtx(mutex_id);
        return flag_id < E_OK ? flag_id : E_SYS;
    }
    latest = (MotionRequest){0};
    pending_reset = 0u;
    driver_ready = true;
    progress = (MotionHealth){
        .stop_generation = 1u,
        .stopped = true,
        .output_braked = true,
        .test_inhibited = APP_TEST != 0,
        .driver_ok = true
    };
    set_reason(&progress, "boot_stopped");
    atomic_store_explicit(&stop_flag, flag_id, memory_order_release);
    atomic_store_explicit(&request_mutex, mutex_id, memory_order_release);
    return E_OK;
}

ER subsystem2_start(void)
{
    if ((atomic_load_explicit(&request_mutex, memory_order_acquire) <= 0) ||
        (atomic_load_explicit(&motion_tid, memory_order_acquire) > 0))
    {
        return E_OBJ;
    }
    ID tid = app_start_task((FP)motion_task, S2_PRI_MOTION,
        TP_PRC2, "Motion");
    if (tid <= 0)
    {
        return tid < E_OK ? tid : E_SYS;
    }
    atomic_store_explicit(&motion_tid, tid, memory_order_release);
    return E_OK;
}
