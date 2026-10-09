#include "navigation_fixtures.h"
#include "../subsystem2/navigation_fixtures.h"
#include "../../app_program/robot_control_policy.h"
#include <math.h>
#include "../../app_program/float_math.h"
#define REQUIRE(condition) do { if (!(condition)) { return false; } } while (0)
RobotNavigationConfig robot_navigation_fixture_config(void)
{
    return (RobotNavigationConfig){.configured = true,
        .follow_mps = 0.2f, .action_mps = 0.2f, .search_mps = 0.1f,
        .junction_min_m = 0.02f, .junction_max_m = 0.1f,
        .straight_m = 0.02f, .turn_deg = 90.0f, .uturn_deg = 180.0f,
        .reacquire_min_m = 0.01f, .reacquire_max_m = 0.1f,
        .approach_ms = 500u, .action_ms = 500u, .reacquire_ms = 200u};
}
static S3IrSnapshot visible(uint32_t now, uint32_t sequence)
{
    return (S3IrSnapshot){.have_sample = true, .acquisition_valid = true,
        .sample_fresh = true, .calibration_valid = true,
        .normalized_valid = true, .guidance_valid = true,
        .sampled_ms = now, .sequence = sequence, .line = IR_LINE_VISIBLE};
}
static S3NavEvent event(S3NavAction action)
{
    return (S3NavEvent){.id = 1u, .generation = 1u, .context = 7u,
        .acquired_ms = 10u, .action = action, .symbol = 'A'};
}
bool robot_navigation_fixture_context(void)
{
    RobotNavigation n;
    robot_navigation_reset(&n, 7u);
    S3NavEvent e = event(S3_NAV_LEFT);
    REQUIRE(!robot_navigation_accept(&n, &e, 2u, 500u, 20u));
    REQUIRE(!robot_navigation_accept(&n, &e, 1u, 500u, 511u));
    e.context = 6u;
    REQUIRE(!robot_navigation_accept(&n, &e, 1u, 500u, 20u));
    e.context = 7u;
    REQUIRE(robot_navigation_accept(&n, &e, 1u, 500u, 20u));
    REQUIRE(!robot_navigation_accept(&n, &e, 1u, 500u, 20u));
    RobotNavigationConfig route = robot_navigation_fixture_config();
    S2NavigationConfig motion = navigation_fixture_config();
    REQUIRE(robot_navigation_config_valid(&route, &motion));
    route.junction_max_m = route.junction_min_m;
    REQUIRE(!robot_navigation_config_valid(&route, &motion));
    return true;
}
bool robot_navigation_fixture_windows(void)
{
    RobotNavigation n;
    robot_navigation_reset(&n, 7u);
    S3NavEvent e = event(S3_NAV_LEFT);
    REQUIRE(robot_navigation_accept(&n, &e, 1u, 500u, 20u));
    RobotNavigationConfig route = robot_navigation_fixture_config();
    S2NavigationConfig motion = navigation_fixture_config();
    S3IrSnapshot ir = visible(40u, 1u);
    S2EncoderSnapshot encoder = {.valid = true, .left_count = 1u,
        .right_count = 1u};
    S2NavigationFeedback feedback = {.encoder_valid = true,
        .sampled_ms = 40u};
    S2NavigationRequest request;
    REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 40u, &request));
    REQUIRE(n.phase == ROBOT_NAV_APPROACH);
    ir = visible(60u, 2u);
    encoder.left_count = 20u;
    encoder.right_count = 20u;
    feedback.sampled_ms = 60u;
    REQUIRE(!robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 60u, &request));
    REQUIRE(n.phase == ROBOT_NAV_FAILED);
    robot_navigation_reset(&n, 7u);
    REQUIRE(robot_navigation_accept(&n, &e, 1u, 500u, 20u));
    ir = visible(520u, 2u);
    encoder.left_count = 3u;
    encoder.right_count = 3u;
    feedback.sampled_ms = 520u;
    ir.line = IR_LINE_BROAD_BLACK;
    REQUIRE(!robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 520u, &request)); /* Deadline precedes fresh junction. */
    return true;
}
bool robot_navigation_fixture_actions(void)
{
    RobotNavigationConfig route = robot_navigation_fixture_config();
    S2NavigationConfig motion = navigation_fixture_config();
    for (unsigned action = S3_NAV_LEFT; action <= S3_NAV_UTURN; ++action)
    {
        RobotNavigation n;
        robot_navigation_reset(&n, 7u);
        S3NavEvent e = event((S3NavAction)action);
        REQUIRE(robot_navigation_accept(&n, &e, 1u, 500u, 20u));
        S3IrSnapshot ir = visible(40u, 1u);
        ir.line = IR_LINE_BROAD_BLACK;
        ir.guidance_valid = false;
        S2EncoderSnapshot encoder = {.valid = true, .left_count = 3u,
            .right_count = 3u};
        S2NavigationFeedback feedback = {.encoder_valid = true,
            .sampled_ms = 40u};
        S2NavigationRequest request;
        REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
            &feedback, 40u, &request));
        REQUIRE(n.phase == ROBOT_NAV_ACTION);
        float expected[] = {0.0f, 90.0f, -90.0f, 0.02f, 180.0f};
        REQUIRE(robot_float_abs(request.target - expected[action]) < 0.0001f);
        REQUIRE(request.id == 1u);
        feedback.action_id = 1u;
        feedback.action_state = S2_ACTION_DONE;
        feedback.sampled_ms = 60u;
        ir = visible(60u, 2u);
        REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
            &feedback, 60u, &request));
        REQUIRE(n.phase == ROBOT_NAV_REACQUIRE);
        REQUIRE(request.acknowledge_id == 1u);
        feedback.left_m = 0.02f;
        feedback.right_m = 0.02f;
        ir = visible(80u, 3u);
        feedback.sampled_ms = 80u;
        REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
            &feedback, 80u, &request));
        ir = visible(100u, 4u);
        feedback.sampled_ms = 100u;
        REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
            &feedback, 100u, &request));
        REQUIRE(n.completed && (n.phase == ROBOT_NAV_IDLE));
        e.generation = 2u; /* Decoder reset may restart encounter IDs. */
        REQUIRE(robot_navigation_accept(&n, &e, 2u, 500u, 100u));
        REQUIRE(n.action_id == 2u); /* Motion IDs never replay in a run. */
    }
    return true;
}
bool robot_navigation_fixture_reacquire(void)
{
    RobotNavigation n;
    robot_navigation_reset(&n, 7u);
    n.phase = ROBOT_NAV_REACQUIRE;
    n.started_ms = UINT32_MAX - 40u;
    n.event = event(S3_NAV_LEFT);
    RobotNavigationConfig route = robot_navigation_fixture_config();
    S2NavigationConfig motion = navigation_fixture_config();
    S3IrSnapshot ir = visible(0u, 1u);
    S2EncoderSnapshot encoder = {.valid = true};
    S2NavigationFeedback feedback = {.encoder_valid = true,
        .sampled_ms = 0u, .left_m = 0.02f, .right_m = 0.02f};
    S2NavigationRequest request;
    REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 0u, &request));
    REQUIRE(n.visible_samples == 1u);
    REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 0u, &request));
    REQUIRE(n.visible_samples == 1u);
    ir = visible(20u, 2u);
    feedback.sampled_ms = 20u;
    REQUIRE(robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 20u, &request));
    REQUIRE(n.completed && (n.phase == ROBOT_NAV_IDLE));
    n.phase = ROBOT_NAV_REACQUIRE;
    n.started_ms = 0u;
    n.visible_samples = 1u;
    ir = visible(200u, 3u);
    feedback.sampled_ms = 200u;
    REQUIRE(!robot_navigation_tick(&n, &route, &motion, &ir, &encoder,
        &feedback, 200u, &request));
    return true;
}
bool robot_navigation_fixture_stop(void)
{
    RobotPolicy policy;
    robot_policy_init(&policy);
    policy.state = ROBOT_NAVIGATING;
    RobotMotionState motion = {.available = true, .driver_ok = true,
        .generation = 1u, .reset_ack = 1u};
    S3IrSnapshot ir = visible(20u, 1u);
    robot_policy_tick(&policy, &ir, &motion, true, true, 20u, true);
    REQUIRE(policy.state == ROBOT_STOPPED);
    REQUIRE(policy.request_stop && policy.cancel_capture);
    REQUIRE(!robot_policy_moving(&policy));
    robot_policy_tick(&policy, &ir, &motion, true, false, 60u, true);
    REQUIRE(policy.state == ROBOT_STOPPED);
    return true;
}
