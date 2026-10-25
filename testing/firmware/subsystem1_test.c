#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include "../../app_program/subsystem1/subsystem1.h"
#include "../../app_program/subsystem1/status_protocol.h"
#include "test_report.h"
#include <string.h>
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
    TEST_SUMMARY("subsystem1");
}
#endif
