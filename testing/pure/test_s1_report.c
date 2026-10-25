/* Host test for app_program/subsystem1/s1_report.[ch] (issues #41, #43 logic). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/subsystem1/s1_report.h"

static unsigned installs;
static uint8_t installed[4];
static bool install_ok = true;
static bool install(const uint8_t l[4]) { ++installs; memcpy(installed, l, 4); return install_ok; }

static mqtt_event_t ev(uint8_t arg) { return (mqtt_event_t){.kind = MQTT_EVENT_HUMP, .arg = arg}; }

static void test_event_log(void)
{
    s1_event_log_t log; mqtt_event_t e;
    s1_log_init(&log);
    assert(!s1_log_pop(&log, &e));
    for (uint8_t i = 0; i < S1_EVENT_CAPACITY; ++i) { mqtt_event_t x = ev(i); s1_log_push(&log, &x); }
    assert(log.count == S1_EVENT_CAPACITY && log.dropped == 0u);
    /* Overflow: the oldest events go, the newest are kept, drops are counted. */
    for (uint8_t i = 0; i < 5u; ++i) { mqtt_event_t x = ev((uint8_t)(100u + i)); s1_log_push(&log, &x); }
    assert(log.count == S1_EVENT_CAPACITY && log.dropped == 5u);
    assert(s1_log_pop(&log, &e) && e.arg == 5u);		/* events 0-4 were overwritten */
    unsigned n = 1u; uint8_t last = e.arg;
    while (s1_log_pop(&log, &e)) { ++n; last = e.arg; }
    assert(n == S1_EVENT_CAPACITY && last == 104u && log.count == 0u);
    /* Wrap of the ring indices: push/pop beyond capacity keeps order. */
    for (uint8_t i = 0; i < 3u * S1_EVENT_CAPACITY; ++i) {
        mqtt_event_t x = ev(i); s1_log_push(&log, &x);
        assert(s1_log_pop(&log, &e) && e.arg == i);
    }
}

static void test_assemble(void)
{
    s1_sources_t s = {.mission_state = 40, .motion_status = 9, .motion_action = 200, .hump_max_code = 255,
                      .front_mm = 60000, .front_valid = true, .barcode_letter = 26, .barcode_action = 12,
                      .faults = {1, 2, 3, 4, 5}, .left_count = 0xABCD, .hump_count = 7, .hump_valid = true};
    mqtt_telemetry_t t; uint8_t frame[MQTT_FRAME_MAX];
    s1_assemble(&s, 9u, &t);
    assert(t.sequence == 9 && t.mission_state == 15 && t.motion_status == 3 && t.motion_action == 63);
    assert(t.hump_max == 127 && t.front_mm == 8191 && t.barcode_letter == MQTT_NO_LETTER && t.barcode_action == 7);
    assert(t.faults[4] == 5 && t.left_count == 0xABCD && t.hump_count == 7 && t.hump_valid);	/* hump count and max carried */
    assert(mqtt_encode_telemetry(frame, sizeof frame, MQTT_TYPE_TELEMETRY, &t) == MQTT_LEN_TELEMETRY);	/* always encodable */
}

static void test_mapping_flow(void)
{
    uint8_t f[MQTT_FRAME_MAX]; mqtt_ack_t ack; bool send;
    mqtt_mapping_t m = {.sequence = 7, .letters = {0, 1, 2, 3}};
    unsigned len = mqtt_encode_mapping(f, sizeof f, &m);

    /* Stopped: installed, then one acknowledgement with the sequence. */
    installs = 0; install_ok = true;
    assert(s1_handle_command(f, len, true, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(send && ack.sequence == 7 && ack.result == MQTT_ACK_OK && installs == 1u && installed[3] == 3);

    /* Moving: refused, nothing installed. */
    installs = 0;
    assert(s1_handle_command(f, len, false, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(send && ack.result == MQTT_ACK_BUSY && installs == 0u);

    /* Mission refuses the letters: acknowledged as invalid, never as OK. */
    install_ok = false;
    assert(s1_handle_command(f, len, true, install, &ack, &send) == S1_COMMAND_MAPPING && ack.result == MQTT_ACK_INVALID);
    install_ok = true;

    /* Duplicate letters (valid header, bad content) never reach Mission. */
    uint8_t dup[MQTT_LEN_MAPPING] = {0x14, 3, 0x20, 0x00, 0x00}; installs = 0;
    assert(s1_handle_command(dup, sizeof dup, true, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(send && ack.sequence == 3 && ack.result == MQTT_ACK_INVALID && installs == 0u);
    uint8_t reserved[MQTT_LEN_MAPPING] = {0x14, 4, 0x20, 0x88, 0x11};
    assert(s1_handle_command(reserved, sizeof reserved, true, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(ack.result == MQTT_ACK_MALFORMED && installs == 0u);

    /* Not a command: no reply to guess at, nothing installed. */
    uint8_t bad_version[MQTT_LEN_MAPPING] = {0x24, 1, 0x20, 0x88, 0x01};
    assert(s1_handle_command(bad_version, sizeof bad_version, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    assert(s1_handle_command(f, len - 1u, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    uint8_t ack_frame[MQTT_LEN_ACK] = {0x15, 1, 0};
    assert(s1_handle_command(ack_frame, sizeof ack_frame, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    assert(s1_handle_command(f, 0u, true, install, &ack, &send) == S1_COMMAND_IGNORED && installs == 0u);

    /* Snapshot request is served and never acknowledged. */
    uint8_t req[MQTT_LEN_SNAPSHOT_REQUEST];
    mqtt_encode_snapshot_request(req, sizeof req, 5);
    assert(s1_handle_command(req, sizeof req, false, install, &ack, &send) == S1_COMMAND_SNAPSHOT && !send);
}

int main(void)
{
    test_event_log();
    test_assemble();
    test_mapping_flow();
    puts("s1_report: PASS");
    return 0;
}
