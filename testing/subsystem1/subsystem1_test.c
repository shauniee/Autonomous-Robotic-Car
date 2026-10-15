#define PROHIBIT_DEF_SIZE_T
#include <stddef.h>
#include "../../app_program/subsystem1/subsystem1.h"
#include "../../app_program/subsystem1/status_protocol.h"
#include "../../app_program/test_report.h"
#include <string.h>
#include "../../app_program/subsystem1/mapping_protocol.h"
#include "../../app_program/subsystem1/mapping_ack.h"
#if APP_TEST
static bool status_request_test(void)
{
    S1StatusRequest request;
    const char payload[] = " { \"request\" : \"status\" }\n";
    unsigned length = sizeof(payload) - 1u;
    bool pass =
        s1_status_request_begin(&request, length) &&
        s1_status_request_data(&request, payload, 7u, false) &&
        s1_status_request_data(&request, payload + 7u, length - 7u, true);
    const char *invalid[] = {"start",
                             "stop",
                             "{\"request\":\"start\"}",
                             "{\"request\":\"stop\"}",
                             "{\"request\":\"drive\"}",
                             "{\"request\":\"status\",\"request\":\"status\"}",
                             "{\"request\":\"status\",\"extra\":0}",
                             "{\"request\":\"status\"}x",
                             "{\"request\":\"status\""};
    for (unsigned i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        length = (unsigned)strlen(invalid[i]);
        pass = s1_status_request_begin(&request, length) && pass;
        pass =
            !s1_status_request_data(&request, invalid[i], length, true) && pass;
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
    S1NetworkStatus status = {.uptime_ms = UINT32_MAX,
                              .connection_sequence = UINT32_MAX,
                              .rejected_requests = UINT32_MAX,
                              .dropped_publish = UINT32_MAX,
                              .telemetry_sequence = UINT32_MAX,
                              .have_snapshot = false};
    char payload[S1_STATUS_MAX + 1u];
    unsigned length = s1_status_encode(&status, payload, sizeof(payload));
    if ((length == 0u) || !strstr(payload, "\"have_snapshot\":false"))
    {
        return false;
    }
    return s1_status_encode(&status, payload, length + 1u) == length &&
           s1_status_encode(&status, payload, length) == 0u &&
           payload[0] == '\0';
}

static bool mapping_test(void)
{
    static S1MappingRequest request;
    char payload[512] =
        "{\"request\":\"barcode_mapping\",\"request_id\":1,\"mapping\":[";
    for (unsigned i = 0u; i < 26u; ++i)
    {
        static const char *const selected[] = {"\"left\"", "\"right\"",
                                               "\"straight\"", "\"u_turn\""};
        if (i != 0u)
        {
            strcat(payload, ",");
        }
        strcat(payload, i < 4u ? selected[i] : "\"unmapped\"");
    }
    strcat(payload, "]}");
    unsigned n = (unsigned)strlen(payload);
    bool pass = s1_mapping_begin(&request, n) &&
                s1_mapping_data(&request, payload, 11u, false) &&
                s1_mapping_data(&request, payload + 11u, n - 11u, true) &&
                request.request_id == 1u &&
                request.mapping[0] == BARCODE_LEFT &&
                request.mapping[3] == BARCODE_U_TURN &&
                request.mapping[25] == BARCODE_UNMAPPED;
    pass = pass && !s1_mapping_begin(&request, 513u);
    const char bad[] = "{\"request\":\"start\"}";
    pass = pass && s1_mapping_begin(&request, sizeof(bad) - 1u) &&
           !s1_mapping_data(&request, bad, sizeof(bad) - 1u, true);
    return pass;
}
void subsystem1_test(void)
{
    S1Telemetry snapshot = {.have_sample = true,
                            .acquired = true,
                            .sampled_ms = 100u,
                            .state = "READY",
                            .line = "unknown",
                            .capture = "waiting"};
    char json[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    unsigned length = s1_telemetry_encode(&snapshot, 120u, json, sizeof(json));
    bool pass = length > 0u && strstr(json, "\"age_ms\":20") &&
                strstr(json, "\"fraction\":null") &&
                !s1_telemetry_encode(&snapshot, 120u, json, 16u);
    TEST_INIT();
    TEST_CHECK("S1-U04", pass,
               "bounded telemetry JSON and original age; no radio test");
    TEST_CHECK(
        "S1-U06", status_request_test(),
        "fragmented status only; malformed and movement requests rejected");
    TEST_CHECK(
        "S1-U07", status_encode_test(),
        "bounded heartbeat/status JSON; exact capacity and max counters");
    TEST_CHECK("S1-U09", mapping_test(),
               "26-entry fragmented mapping and bounded rejection");
    S1AckQueue queue = {0};
    int slot = s1_ack_reserve(&queue, 17u);
    bool ack = slot >= 0 && s1_ack_reserved(&queue);
    s1_ack_ready(&queue, (unsigned)slot, 2u, 8u);
    uint32_t first = s1_ack_send(&queue, (unsigned)slot);
    s1_ack_complete(&queue, first, false);
    ack = ack && queue.slots[slot].state == ACK_READY;
    uint32_t second = s1_ack_send(&queue, (unsigned)slot);
    s1_ack_reconnect(&queue);
    uint32_t third = s1_ack_send(&queue, (unsigned)slot);
    s1_ack_complete(&queue, second, true);
    ack = ack && third != second && queue.slots[slot].state == ACK_FLIGHT;
    s1_ack_complete(&queue, third, true);
    ack = ack && queue.slots[slot].state == ACK_EMPTY;
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        ack = s1_ack_reserve(&queue, 30u + i) >= 0 && ack;
    }
    TEST_CHECK(
        "S1-U10", ack && s1_ack_reserve(&queue, 99u) < 0,
        "ack failure/reconnect retry, stale callback, bounded backpressure");
    const char *topics[] = {"t", "h", "s", "r", "m", "a", "e"};
    bool topic = s1_topics_valid(topics, 7u);
    for (unsigned i = 0u; i < 7u; ++i)
    {
        const char *saved = topics[i];
        topics[i] = "bad/#";
        topic = !s1_topics_valid(topics, 7u) && topic;
        topics[i] = saved;
    }
    topics[6] = topics[4];
    TEST_CHECK("S1-U11", topic && !s1_topics_valid(topics, 7u),
               "every topic rejects wildcards and duplicates");
    length = s1_robot_status_encode(&snapshot, 120u, json, sizeof(json));
    bool summary = length > 0u && strstr(json, "\"online\":true") &&
                   strstr(json, "\"mission\":") && strstr(json, "\"schema\":3");
    RunHistory history = {0};
    for (unsigned i = 0u; i < 10u; ++i)
    {
        (void)run_history_append(&history, (RunEvent){.value = (int32_t)i});
    }
    length = s1_events_encode(&history, false, 8u, json, sizeof(json));
    TEST_CHECK("S1-U12",
               summary && length > 0u && strstr(json, "\"count\":10") &&
                   strstr(json, "\"dropped\":2") && strstr(json, "[9,") &&
                   !strstr(json, "[8,"),
               "robot summary and bounded event history/cursor");
    const char event_request[] =
        "{\"request\":\"events\",\"kind\":\"barcode\",\"after_id\":4294967295}";
    S1StatusRequest query;
    TEST_CHECK("S1-U13",
               s1_status_request_begin(&query, sizeof(event_request) - 1u) &&
                   s1_status_request_data(&query, event_request,
                                          sizeof(event_request) - 1u, true) &&
                   query.events && query.barcode &&
                   query.after_id == UINT32_MAX,
               "bounded read-only event retrieval request");
    TEST_SUMMARY("subsystem1");
}
#endif
