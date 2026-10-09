#include "navigation_fixtures.h"
#include <math.h>
#include "../../app_program/float_math.h"
#define REQUIRE(condition) do { if (!(condition)) { return false; } } while (0)
S2NavigationConfig navigation_fixture_config(void)
{
    /* Synthetic values for software fixtures, not robot calibration. */
    return (S2NavigationConfig){.configured = true,
        .diameter_m = 0.3183098862f, .track_m = 0.2f, .counts_per_rev = 100.0f,
        .left_polarity = 1, .right_polarity = 1,
        .left_motor_polarity = 1, .right_motor_polarity = 1,
        .left_pid = {.kp = 1.0f, .ki = 0.1f, .kd = 0.0f,
            .feedforward = 0.1f, .integral_limit = 1.0f},
        .right_pid = {.kp = 1.0f, .ki = 0.1f, .kd = 0.0f,
            .feedforward = 0.1f, .integral_limit = 1.0f},
        .max_speed_mps = 2.0f,
        .tolerance_m = 0.001f, .stall_duty = 0.05f, .stall_ms = 200u};
}
bool navigation_fixture_quadrature(void)
{
    const unsigned forward[] = {0u, 2u, 3u, 1u, 0u};
    for (unsigned i = 0u; i < 4u; ++i)
    {
        REQUIRE(s2_encoder_transition(forward[i], forward[i + 1u]) == 1);
        REQUIRE(s2_encoder_transition(forward[i + 1u], forward[i]) == -1);
    }
    REQUIRE(s2_encoder_transition(0u, 3u) == 2);
    REQUIRE(s2_encoder_transition(4u, 0u) == 2);
    REQUIRE(s2_encoder_transition(0u, 0u) == 0);
    S2NavigationConfig c = navigation_fixture_config();
    REQUIRE(s2_navigation_config_valid(&c));
    c.counts_per_rev = 0.0f;
    REQUIRE(!s2_navigation_config_valid(&c));
    c = navigation_fixture_config();
    c.left_pid.kp = NAN;
    REQUIRE(!s2_navigation_config_valid(&c));
    return true;
}
bool navigation_fixture_actions(void)
{
    S2NavigationConfig config = navigation_fixture_config();
    S2NavigationController c;
    S2EncoderSnapshot e = {.valid = true};
    S2NavigationRequest r = {.kind = S2_SPEED};
    float left, right;
    s2_navigation_reset(&c);
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 0u, &left, &right));
    r = (S2NavigationRequest){.kind = S2_DISTANCE, .id = 1u,
        .target = 0.02f, .speed_mps = 1.0f, .timeout_ms = 100u};
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 20u, &left, &right));
    REQUIRE((left <= 0.35f) && (right <= 0.35f));
    e.left_count = 2u;
    e.right_count = 2u;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 40u, &left, &right));
    REQUIRE(c.feedback.action_state == S2_ACTION_DONE);
    REQUIRE((left == 0.0f) && (right == 0.0f));
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 60u, &left, &right));
    REQUIRE(c.started_ms == 20u);
    r = (S2NavigationRequest){.kind = S2_SPEED, .left_mps = 1.0f};
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 80u, &left, &right));
    REQUIRE((left == 0.0f) && (c.feedback.action_state == S2_ACTION_DONE));
    r.acknowledge_id = 1u;
    r.left_mps = 0.0f;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 100u, &left, &right));
    REQUIRE(c.feedback.action_state == S2_ACTION_IDLE);
    r = (S2NavigationRequest){.kind = S2_TURN, .id = 2u,
        .target = 90.0f, .speed_mps = 1.0f, .timeout_ms = 100u};
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 120u, &left, &right));
    REQUIRE((left < 0.0f) && (right > 0.0f));
    e.left_count -= 16u;
    e.right_count += 16u;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 140u, &left, &right));
    REQUIRE(c.feedback.action_state == S2_ACTION_DONE);
    r.target = 180.0f;
    REQUIRE(!s2_navigation_tick(&c, &config, &e, &r, 160u, &left, &right));
    REQUIRE((left == 0.0f) && (right == 0.0f));
    /* All turn orientations, including U-turn, use signed wheel goals. */
    const float turns[] = {-90.0f, 180.0f};
    for (unsigned i = 0u; i < 2u; ++i)
    {
        s2_navigation_reset(&c);
        e = (S2EncoderSnapshot){.valid = true};
        r = (S2NavigationRequest){.kind = S2_SPEED};
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 0u, &left, &right));
        r = (S2NavigationRequest){.kind = S2_TURN, .id = 1u,
            .target = turns[i], .speed_mps = 1.0f, .timeout_ms = 200u};
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 20u, &left, &right));
        int travel = i == 0u ? -16 : 32;
        e.left_count = 0u - (uint32_t)travel;
        e.right_count = (uint32_t)travel;
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 40u, &left, &right));
        REQUIRE(c.feedback.action_state == S2_ACTION_DONE);
    }
    return true;
}
bool navigation_fixture_deadlines(void)
{
    S2NavigationConfig config = navigation_fixture_config();
    S2NavigationController c;
    s2_navigation_reset(&c);
    S2EncoderSnapshot e = {.valid = true};
    S2NavigationRequest r = {.kind = S2_SPEED};
    float left, right;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, UINT32_MAX - 30u,
        &left, &right));
    r = (S2NavigationRequest){.kind = S2_DISTANCE, .id = 1u,
        .target = 1.0f, .speed_mps = 1.0f, .timeout_ms = 100u};
    for (uint32_t elapsed = 20u; elapsed < 120u; elapsed += 20u)
    {
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r,
            UINT32_MAX - 30u + elapsed, &left, &right));
    }
    REQUIRE(!s2_navigation_tick(&c, &config, &e, &r, 89u, &left, &right));
    REQUIRE(c.feedback.action_state == S2_ACTION_FAILED);
    s2_navigation_reset(&c);
    r = (S2NavigationRequest){.kind = S2_SPEED};
    e.left_count = UINT32_MAX - 1u;
    e.right_count = UINT32_MAX - 1u;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 0u, &left, &right));
    e.left_count = 1u;
    e.right_count = 1u;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 20u, &left, &right));
    REQUIRE(robot_float_abs(c.feedback.left_m - 0.03f) < 0.0001f);
    REQUIRE(!s2_navigation_tick(&c, &config, &e, &r, 81u, &left, &right));
    return true;
}
bool navigation_fixture_stall(void)
{
    S2NavigationConfig config = navigation_fixture_config();
    S2NavigationController c;
    s2_navigation_reset(&c);
    S2EncoderSnapshot e = {.valid = true};
    S2NavigationRequest r = {.kind = S2_SPEED};
    float left, right;
    REQUIRE(s2_navigation_tick(&c, &config, &e, &r, 0u, &left, &right));
    for (uint32_t t = 20u; t <= 300u; t += 20u)
    {
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r, t, &left, &right));
    }
    r.left_mps = 1.0f;
    r.right_mps = 1.0f;
    for (uint32_t t = 320u; t < 520u; t += 20u)
    {
        REQUIRE(s2_navigation_tick(&c, &config, &e, &r, t, &left, &right));
        REQUIRE(robot_float_abs(left) <= 0.35f);
    }
    REQUIRE(!s2_navigation_tick(&c, &config, &e, &r, 520u, &left, &right));
    REQUIRE((left == 0.0f) && (right == 0.0f));
    s2_navigation_reset(&c);
    e.valid = false;
    REQUIRE(!s2_navigation_tick(&c, &config, &e, &r, 0u, &left, &right));
    return true;
}
