#include "robot/comms_protocol.h"
#include "robot/controller.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *session = "0123456789abcdef";
static const char *start = "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":1000,\"command\":\"start\"}";

static bool feed(CommandInbox *inbox, const char *payload, uint32_t now) {
    size_t n = strlen(payload);
    command_inbox_begin(inbox, n, true);
    return command_inbox_data(inbox, payload, n, true, now);
}

int main(void) {
    CommandInbox inbox;
    command_inbox_reset(&inbox, session);
    /* Split at every byte boundary, including empty end fragments. */
    for (size_t split = 0; split <= strlen(start); ++split) {
        command_inbox_reset(&inbox, session);
        command_inbox_begin(&inbox, strlen(start), true);
        assert(command_inbox_data(&inbox, start, split, false, 1100));
        assert(command_inbox_take(&inbox, 1100) == COMMAND_NONE);
        assert(command_inbox_data(&inbox, start + split, strlen(start) - split, true, 1100));
        assert(command_inbox_take(&inbox, 1100) == COMMAND_START);
        assert(command_inbox_take(&inbox, 1100) == COMMAND_NONE);
        assert(!feed(&inbox, start, 1100)); /* Duplicate ID. */
    }
    command_inbox_reset(&inbox, session);
    assert(!feed(&inbox, start, 6001)); /* Expired. */
    assert(!feed(&inbox, start, 999)); /* Future uptime. */
    assert(feed(&inbox, start, 1100));
    assert(command_inbox_take(&inbox, 6001) == COMMAND_NONE); /* Expired in queue. */
    command_inbox_reset(&inbox, "fedcba9876543210");
    assert(!feed(&inbox, start, 1100)); /* Old retained command after reconnect. */

    const char *bad[] = {
        "start", "{}", "{", "null", "stop ",
        "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":1000,\"command\":\"start\"}junk",
        "{\"session\":\"0123456789abcdef\",\"id\":4294967296,\"issued_ms\":1000,\"command\":\"start\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":-1,\"issued_ms\":1000,\"command\":\"start\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":01,\"issued_ms\":1000,\"command\":\"start\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":1.5,\"issued_ms\":1000,\"command\":\"start\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":1,\"id\":2,\"command\":\"start\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":1000,\"command\":\"turn\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":1000,\"command\":\"st\\u0061rt\"}",
        "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":1000,\"extra\":\"start\"}"
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        command_inbox_reset(&inbox, session);
        assert(!feed(&inbox, bad[i], 1100));
        assert(command_inbox_take(&inbox, 1100) == COMMAND_NONE);
    }
    command_inbox_begin(&inbox, COMMS_COMMAND_MAX + 1, true);
    assert(!command_inbox_data(&inbox, start, strlen(start), true, 1100));
    command_inbox_begin(&inbox, strlen(start), false);
    assert(!command_inbox_data(&inbox, start, strlen(start), true, 1100));
    command_inbox_begin(&inbox, 3, true);
    assert(!command_inbox_data(&inbox, "stop", 4, true, 1100));
    command_inbox_begin(&inbox, 5, true);
    assert(!command_inbox_data(&inbox, "stop", 4, true, 1100));
    const char embedded_nul[] = {'s', 't', 'o', 'p', 0, 'x'};
    command_inbox_begin(&inbox, sizeof(embedded_nul), true);
    assert(!command_inbox_data(&inbox, embedded_nul, sizeof(embedded_nul), true, 1100));

    command_inbox_reset(&inbox, session);
    assert(feed(&inbox, start, 1100));
    assert(feed(&inbox, "stop", 1100));
    assert(feed(&inbox, "{ \"command\" : \"start\", \"issued_ms\":1000, \"id\":2, \"session\":\"0123456789abcdef\" }", 1100));
    assert(command_inbox_take(&inbox, 1100) == COMMAND_STOP); /* Stop wins a batch. */
    assert(command_inbox_take(&inbox, 1100) == COMMAND_NONE);
    command_inbox_reset(&inbox, session);
    assert(feed(&inbox, "{\"session\":\"0123456789abcdef\",\"id\":1,\"issued_ms\":4294967290,\"command\":\"start\"}", 10));
    assert(command_inbox_take(&inbox, 10) == COMMAND_START); /* Uptime wrap. */

    VehicleController c;
    controller_init(&c);
    SensorFrame sensors = {0};
    MotionFeedback motion = {0};
    char json[COMMS_TELEMETRY_MAX];
    assert(comms_encode_telemetry(json, sizeof(json), &c, &sensors, &motion, 1000));
    assert(strstr(json, "\"front_distance_m\":null"));
    assert(strstr(json, "\"left_mps\":null"));
    sensors.obstacle.status = (SampleStatus){true, 900};
    sensors.obstacle.front_distance_m = NAN;
    c.reason = "quote\"\\\n";
    assert(comms_encode_telemetry(json, sizeof(json), &c, &sensors, &motion, 1000));
    assert(strstr(json, "\"front_distance_m\":null"));
    assert(strstr(json, "\"age_ms\":100"));
    assert(strstr(json, "quote\\\"\\\\\\u000a"));
    /* Write one fixture for an independent JSON parser to validate. */
    FILE *fixture = fopen("build/telemetry-fixture.json", "w");
    assert(fixture);
    fputs(json, fixture);
    fclose(fixture);
    assert(!comms_encode_telemetry(json, 16, &c, &sensors, &motion, 1000));
    assert(json[0] == '\0');
    assert(!comms_encode_telemetry(NULL, 0, &c, &sensors, &motion, 1000));
    puts("Communication protocol checks passed.");
    return 0;
}
