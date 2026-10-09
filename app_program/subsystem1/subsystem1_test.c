#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include "subsystem1.h"
#include "status_protocol.h"
#include "telemetry_adapter.h"
#include "../test_report.h"
#include <string.h>
#include <math.h>
#if APP_TEST
static bool status_request_test(void)
{
    S1StatusRequest request;
    const char payload[] = " { \"request\" : \"status\" }\n";
    unsigned length = sizeof(payload) - 1u;
    bool pass = s1_status_request_begin(&request, length) &&
        s1_status_request_data(&request, payload, 7u, false) &&
        s1_status_request_data(&request, payload + 7u, length - 7u, true);
    const char *invalid[] = {
        "start", "stop", "{\"request\":\"start\"}",
        "{\"request\":\"stop\"}", "{\"request\":\"drive\"}",
        "{\"request\":\"status\",\"request\":\"status\"}",
        "{\"request\":\"status\",\"extra\":0}",
        "{\"request\":\"status\"}x", "{\"request\":\"status\""
    };
    for (unsigned i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        length = (unsigned)strlen(invalid[i]);
        pass = s1_status_request_begin(&request, length) && pass;
        pass = !s1_status_request_data(&request, invalid[i], length, true) && pass;
    }
    pass = !s1_status_request_begin(&request, 0u) && pass;
    pass = !s1_status_request_begin(&request, S1_REQUEST_MAX + 1u) && pass;
    pass = s1_status_request_begin(&request, 3u) && pass;
    pass = !s1_status_request_data(&request, "four", 4u, true) && pass;
    pass = s1_status_request_begin(&request, 3u) && pass;
    pass = !s1_status_request_data(&request, "{}", 2u, true) && pass;
    pass = s1_status_request_begin(&request, 3u) && pass;
    pass = !s1_status_request_data(&request, "{}", 3u, true) && pass;
    s1_status_request_reset(&request);
    return !request.receiving && pass;
}

static bool status_encode_test(void)
{
    S1NetworkStatus status = {
        .uptime_ms = UINT32_MAX, .connection_sequence = UINT32_MAX,
        .rejected_requests = UINT32_MAX, .dropped_publish = UINT32_MAX,
        .telemetry_sequence = UINT32_MAX, .have_snapshot = false
    };
    char payload[S1_STATUS_MAX + 1u];
    unsigned length = s1_status_encode(&status, payload, sizeof(payload));
    if ((length == 0u) || !strstr(payload, "\"have_snapshot\":false"))
    {
        return false;
    }
    return s1_status_encode(&status, payload, length + 1u) == length &&
           s1_status_encode(&status, payload, length) == 0u && payload[0] == '\0';
}

static bool telemetry_adapter_test(void)
{
    S3IrSnapshot ir = {
        .sequence = 7u, .sampled_ms = 100u, .have_sample = true,
        .acquisition_valid = true, .sample_fresh = true,
        .normalized_valid = true, .normalized_left = -1.0f,
        .normalized_right = 1.0f, .error = 1.0f,
        .line = IR_LINE_VISIBLE, .capture_result = IR_CAPTURE_WHITE_DONE
    };
    RobotControlSnapshot control = {
        .observed_ms = 110u,
        .left_duty = 0.1f, .right_duty = 0.2f, .motion_enabled = true,
        .state = "FOLLOW_LINE", .reason = "tracking"
    };
    MotionHealth motion = {
        .sequence = 1u, .serviced_ms = 110u, .driver_ok = true,
        .applied_left = 0.05f, .applied_right = 0.15f,
        .test_inhibited = true
    };
    S1Telemetry snapshot;
    if (!s1_telemetry_map(&ir, &control, &motion, 120u, &snapshot) ||
        (snapshot.ir_sequence != 7u) || (snapshot.sampled_ms != 100u) ||
        (snapshot.observed_ms != 110u) ||
        (snapshot.left_fraction != 0) ||
        (snapshot.right_fraction != 1000) || (snapshot.error != 1000) ||
        (snapshot.left_duty != 100) || (snapshot.right_duty != 200) ||
        (snapshot.applied_left != 50) || (snapshot.applied_right != 150) ||
        !snapshot.motion_available || !snapshot.test_inhibited)
    {
        return false;
    }
    if (!s1_telemetry_map(&ir, &control, NULL, 120u, &snapshot) ||
        snapshot.motion_available || (snapshot.left_duty != 100))
    {
        return false;
    }
    control.observed_ms = 59u;
    if (s1_telemetry_map(&ir, &control, &motion, 120u, &snapshot))
    {
        return false;
    }
    control.observed_ms = UINT32_MAX - 2u;
    if (!s1_telemetry_map(&ir, &control, &motion, 2u, &snapshot) ||
        (snapshot.observed_ms != UINT32_MAX - 2u))
    {
        return false;
    }
    control.observed_ms = 110u;
    ir.normalized_valid = false;
    if (!s1_telemetry_map(&ir, &control, &motion, 120u, &snapshot) ||
        snapshot.reading_valid)
    {
        return false;
    }
    ir.normalized_valid = true;
    ir.normalized_left = INFINITY;
    if (s1_telemetry_map(&ir, &control, &motion, 120u, &snapshot))
    {
        return false;
    }
    ir.normalized_left = -1.0f;
    control.left_duty = MOTION_MAX_DUTY + 0.1f;
    return !s1_telemetry_map(&ir, &control, &motion, 120u, &snapshot) &&
        !s1_telemetry_map(NULL, &control, &motion, 120u, &snapshot) &&
        !s1_telemetry_map(&ir, NULL, &motion, 120u, &snapshot);
}

static bool motion_telemetry_test(void)
{
    S1Telemetry snapshot = {
        .sequence = UINT32_MAX, .ir_sequence = UINT32_MAX,
        .observed_ms = UINT32_MAX, .sampled_ms = 0u,
        .have_sample = true, .acquired = true, .reading_valid = true,
        .raw_left = 4095u, .raw_right = 4095u,
        .black_set = true, .white_set = true,
        .black_left = 4095u, .black_right = 4095u,
        .white_left = 4095u, .white_right = 4095u,
        .left_fraction = 1000, .right_fraction = 1000, .error = -1000,
        .left_duty = 350, .right_duty = 350,
        .applied_left = 350, .applied_right = 350,
        .motion_available = true, .motion_serviced_ms = 0u
    };
    memset(snapshot.state, 's', sizeof(snapshot.state) - 1u);
    memset(snapshot.line, 'l', sizeof(snapshot.line) - 1u);
    memset(snapshot.reason, 'r', sizeof(snapshot.reason) - 1u);
    memset(snapshot.capture, 'c', sizeof(snapshot.capture) - 1u);
    char json[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    unsigned length = s1_telemetry_encode(&snapshot, UINT32_MAX,
        json, sizeof(json));
    if ((length == 0u) || (length > S1_TELEMETRY_PAYLOAD_MAX) ||
        !strstr(json, "\"ir_sequence\":4294967295") ||
        !strstr(json, "\"motion\":{\"age_ms\":4294967295") ||
        !strstr(json, "\"applied_duty\":{\"left\":0.350") ||
        (s1_telemetry_encode(&snapshot, UINT32_MAX,
            json, length + 1u) != length) ||
        (s1_telemetry_encode(&snapshot, UINT32_MAX,
            json, length) != 0u) || (json[0] != '\0'))
    {
        return false;
    }
    snapshot.sampled_ms = UINT32_MAX - 2u;
    snapshot.motion_serviced_ms = UINT32_MAX - 2u;
    if (!s1_telemetry_encode(&snapshot, 2u, json, sizeof(json)) ||
        !strstr(json, "\"sample_age_ms\":5") ||
        !strstr(json, "\"motion\":{\"age_ms\":5"))
    {
        return false;
    }
    snapshot.motion_available = false;
    return s1_telemetry_encode(&snapshot, 2u, json, sizeof(json)) > 0u &&
        strstr(json, "\"motion\":null");
}

void subsystem1_test(void)
{
    S1Telemetry snapshot = {
        .have_sample = true, .acquired = true, .sampled_ms = 100u,
        .state = "READY", .line = "unknown", .capture = "waiting"
    };
    char json[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    unsigned length = s1_telemetry_encode(&snapshot, 120u, json, sizeof(json));
    bool pass = length > 0u && strstr(json, "\"sample_age_ms\":20") &&
        strstr(json, "\"fractions\":null") &&
        !s1_telemetry_encode(&snapshot, 120u, json, 16u);
    TEST_INIT();
    TEST_CHECK("S1-U04", pass,
               "bounded telemetry JSON and original age; no radio test");
    TEST_CHECK("S1-U06", status_request_test(),
               "fragmented status only; malformed and movement requests rejected");
    TEST_CHECK("S1-U07", status_encode_test(),
               "bounded heartbeat/status JSON; exact capacity and max counters");
    TEST_CHECK("S1-U09", telemetry_adapter_test(),
               "real snapshot mapping; fractions preserved; no invented output");
    TEST_CHECK("S1-U10", motion_telemetry_test(),
               "bounded additive schema; exact capacity, null motion and wrap");
    TEST_SUMMARY("subsystem1");
}
#endif
