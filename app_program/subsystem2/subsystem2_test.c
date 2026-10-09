#define PROHIBIT_DEF_SIZE_T
#include <math.h>
#include "subsystem2.h"
#include "drv_motor.h"
#include "../test_report.h"
#include <tk/smp.h>

#if APP_TEST
static uint32_t fixture_compare_exchange(UW *state, uint32_t expected,
    uint32_t desired)
{
#if TK_SUPPORT_SMP
    return (uint32_t)atomic_cmpxchg(state, desired, expected);
#else
    uint32_t previous = *state;
    if (previous == expected)
    {
        *state = desired;
    }
    return previous;
#endif
}

static bool generation_fixture(void)
{
    /* Local state only: never reset/drive the live Motion service. */
    UW state = MOTION_BOOT_STATE;
    if (motion_reset_allowed(state, 1u, 0u, true, true) ||
        !motion_reset_allowed(state, 1u, 1u, true, true))
    {
        return false;
    }
    if (fixture_compare_exchange(&state, MOTION_BOOT_STATE, 2u) !=
        MOTION_BOOT_STATE)
    {
        return false;
    }
    MotionRequest request = {
        .left = 0.10f,
        .right = 0.10f,
        .sampled_ms = 100u,
        .enabled = true,
        .generation = 1u
    };
    if (!motion_request_current(request, state, 100u))
    {
        return false;
    }
    /* Stop wins against a reset carrying the earlier stopped state. */
    state = motion_stop_next(state);
    if ((state != 5u) ||
        (fixture_compare_exchange(&state, MOTION_BOOT_STATE, 2u) != 5u) ||
        (state != 5u) || motion_request_current(request, state, 100u))
    {
        return false;
    }
    if (fixture_compare_exchange(&state, 5u, 4u) != 5u ||
        motion_request_current(request, state, 100u))
    {
        return false;
    }
    request.generation = 2u;
    if (!motion_request_current(request, state, 100u))
    {
        return false;
    }
    /* A stop after successful reset invalidates that armed token as well. */
    state = motion_stop_next(state);
    return (state == 7u) && motion_stop_latched(state) &&
        !motion_request_current(request, state, 100u) &&
        (motion_stop_next(UINT32_MAX) == UINT32_MAX);
}

static bool live_inhibited_output(void)
{
    for (unsigned attempt = 0u; attempt < 3u; ++attempt)
    {
        MotionHealth health;
        float left;
        float right;
        if (s2_motion_copy(&health) && (health.sequence > 0u) &&
            health.test_inhibited && health.driver_ok &&
            health.output_braked && (health.applied_left == 0.0f) &&
            (health.applied_right == 0.0f) &&
            drv_motor_output_copy(&left, &right) &&
            (left == 0.0f) && (right == 0.0f) &&
            (!health.stopped ||
             (health.brake_ack_generation == health.stop_generation)))
        {
            return true;
        }
        if (tk_dly_tsk(20u) < E_OK)
        {
            return false;
        }
    }
    return false;
}

void subsystem2_test(void)
{
    MotionRequest request = {
        .left = 0.10f,
        .right = 0.10f,
        .sampled_ms = 100u,
        .enabled = true,
        .generation = 1u
    };
    TEST_INIT();
    TEST_CHECK("S2-U03",
        motion_request_valid(request, false, 160u) &&
        !motion_request_valid(request, false, 161u) &&
        !motion_request_valid(request, true, 100u),
        "request freshness and stop gate; outputs inhibited");
    TEST_CHECK("S2-U06",
        motion_duties_valid(0.0f, MOTION_MAX_DUTY) &&
        !motion_duties_valid(-0.001f, 0.10f) &&
        !motion_duties_valid(0.10f, MOTION_MAX_DUTY + 0.001f) &&
        !motion_duties_valid(NAN, 0.10f) &&
        !motion_duties_valid(0.10f, INFINITY),
        "forward duty bounds reject NaN, infinity and invalid ranges");
    request.enabled = false;
    bool disabled_rejected = !motion_request_valid(request, false, 100u);
    request.enabled = true;
    request.sampled_ms = UINT32_MAX - 20u;
    TEST_CHECK("S2-U07",
        disabled_rejected &&
        motion_request_valid(request, false, 39u) &&
        !motion_request_valid(request, false, 40u),
        "disabled request and 60/61 ms expiry across clock wrap");
    TEST_CHECK("S2-U08", generation_fixture(),
        "local CAS fixture: newer stop beats reset and stale commands");
    TEST_CHECK("S2-U09",
        !motion_reset_allowed(MOTION_BOOT_STATE, 1u, 1u, false, true) &&
        !motion_reset_allowed(MOTION_BOOT_STATE, 1u, 1u, true, false) &&
        !motion_reset_allowed(MOTION_BOOT_STATE, 2u, 1u, true, true) &&
        !motion_reset_allowed(2u, 1u, 1u, true, true) &&
        !motion_reset_allowed(UINT32_MAX, MOTION_MAX_GENERATION,
            MOTION_MAX_GENERATION, true, true) &&
        !s2_motion_copy(NULL) && (s2_health(NULL) == E_PAR),
        "reset needs matching acknowledged brake; invalid copies refused");
    TEST_CHECK("S2-U10", live_inhibited_output(),
        "live TEST Motion progress and checked zero PWM; no wheel claim");
    request.sampled_ms = 100u;
    request.generation = 1u;
    TEST_CHECK("S2-U13",
        !motion_publication_allowed(request, MOTION_BOOT_STATE, 1u, true) &&
        !motion_publication_allowed(request, 2u, 0u, true) &&
        !motion_publication_allowed(request, 4u, 2u, true) &&
        !motion_publication_allowed(request, 2u, 1u, false) &&
        motion_publication_allowed(request, 2u, 1u, true),
        "enabled publication rejected while stopped or before reset ack");
    TEST_SUMMARY("subsystem2");
}
#endif
