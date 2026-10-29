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
    assert(s1_log_count(&log) == S1_EVENT_CAPACITY && log.dropped == 0u);
    /* Overflow: the oldest events go, the newest are kept, drops are counted. */
    for (uint8_t i = 0; i < 5u; ++i) { mqtt_event_t x = ev((uint8_t)(100u + i)); s1_log_push(&log, &x); }
    assert(s1_log_count(&log) == S1_EVENT_CAPACITY && log.dropped == 5u);
    assert(s1_log_pop(&log, &e) && e.arg == 5u);		/* events 0-4 were overwritten */
    unsigned n = 1u; uint8_t last = e.arg;
    while (s1_log_pop(&log, &e)) { ++n; last = e.arg; }
    assert(n == S1_EVENT_CAPACITY && last == 104u && s1_log_count(&log) == 0u);
    /* Wrap of the ring indices: push/pop beyond capacity keeps order. */
    for (uint8_t i = 0; i < 3u * S1_EVENT_CAPACITY; ++i) {
        mqtt_event_t x = ev(i); s1_log_push(&log, &x);
        assert(s1_log_pop(&log, &e) && e.arg == i);
    }
}

static void assert_event_equal(const mqtt_event_t *expected, const mqtt_event_t *actual)
{
    assert(actual->kind == expected->kind && actual->arg == expected->arg);
    assert(actual->time_ms == expected->time_ms);
    assert(actual->left_count == expected->left_count && actual->right_count == expected->right_count);
    assert(actual->arg2 == expected->arg2);
}

static void test_event_packing(void)
{
    /* All event kinds retain the complete time, encoder counts and arg2.
       The 16-bit obstacle depth must survive, even though barcode action
       values only require three bits. */
    const mqtt_event_t samples[] = {
        {.kind = MQTT_EVENT_BARCODE, .arg = 25, .arg2 = 7,
         .time_ms = 0xFFFFu, .left_count = 0xFFFFu, .right_count = 0x1234u},
        {.kind = MQTT_EVENT_HUMP, .arg = 127, .arg2 = 0,
         .time_ms = 0, .left_count = 0, .right_count = 0xFFFFu},
        {.kind = MQTT_EVENT_OBSTACLE, .arg = 0, .arg2 = 0xFFFFu,
         .time_ms = 0xABCDu, .left_count = 0x9876u, .right_count = 0xFFFFu},
        {.kind = MQTT_EVENT_TURN, .arg = 7, .arg2 = 0,
         .time_ms = 1234u, .left_count = 500u, .right_count = 501u}
    };
    s1_event_log_t log;
    s1_log_init(&log);
    for (unsigned i = 0; i < sizeof samples / sizeof samples[0]; ++i)
    {
        mqtt_event_t roundtrip;
        uint8_t before[MQTT_LEN_EVENT], after[MQTT_LEN_EVENT];
        assert(mqtt_encode_event(before, sizeof before, &samples[i]) == MQTT_LEN_EVENT);
        assert(s1_log_push(&log, &samples[i]));
        assert(s1_log_pop(&log, &roundtrip));
        assert_event_equal(&samples[i], &roundtrip);
        assert(mqtt_encode_event(after, sizeof after, &roundtrip) == MQTT_LEN_EVENT);
        assert(memcmp(before, after, MQTT_LEN_EVENT) == 0); /* schema v2 unchanged */
    }
    assert(s1_log_count(&log) == 0u);

    /* Reject out-of-range inputs rather than silently dropping the high bit.
       A rejected event must not disturb order, count or drop statistics. */
    mqtt_event_t bad = {.kind = MQTT_EVENT_HUMP, .arg = 128};
    assert(!s1_log_push(&log, &bad));
    bad.kind = 0;
    bad.arg = 0;
    assert(!s1_log_push(&log, &bad));
    bad.kind = MQTT_EVENT_COUNT;
    assert(!s1_log_push(&log, &bad));
    assert(s1_log_count(&log) == 0u && log.dropped == 0u);

    /* Full-ring overwrite and the drop counter's saturation remain intact. */
    for (unsigned i = 0; i < S1_EVENT_CAPACITY; ++i)
    {
        mqtt_event_t x = ev((uint8_t)i);
        assert(s1_log_push(&log, &x));
    }
    log.dropped = 0xFFFFFFFFu;
    mqtt_event_t newest = ev(42);
    assert(s1_log_push(&log, &newest));
    assert(log.dropped == 0xFFFFFFFFu && s1_log_count(&log) == S1_EVENT_CAPACITY);
    mqtt_event_t popped;
    assert(s1_log_pop(&log, &popped) && popped.arg == 1u);

    printf("s1_event storage: %zu-byte entry, %zu-byte ring (host layout)\n",
           sizeof(s1_packed_event_t), sizeof(s1_event_log_t));
}

static void test_assemble(void)
{
    mqtt_telemetry_t t = {.sequence = 9, .mission_state = 40, .motion_status = 9, .motion_action = 200,
                          .line_status = 9, .hump_max = 255, .front_mm = 60000, .front_valid = true,
                          .barcode_letter = 26, .barcode_action = 12, .faults = {1, 2, 3, 4, 5, 6},
                          .left_count = 0xABCD, .hump_count = 7, .hump_valid = true, .obstacle_width_mm = 65535};
    uint8_t frame[MQTT_FRAME_MAX];
    s1_clamp(&t);
    assert(t.sequence == 9 && t.mission_state == 15 && t.motion_status == 3 && t.motion_action == 63 && t.line_status == 3);
    assert(t.hump_max == 127 && t.front_mm == 8191 && t.barcode_letter == MQTT_NO_LETTER && t.barcode_action == 7);
    assert(t.faults[5] == 6 && t.left_count == 0xABCD && t.hump_count == 7 && t.hump_valid && t.obstacle_width_mm == 65535);
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
    uint8_t dup[MQTT_LEN_MAPPING] = {0x24, 3, 0x20, 0x00, 0x00}; installs = 0;
    assert(s1_handle_command(dup, sizeof dup, true, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(send && ack.sequence == 3 && ack.result == MQTT_ACK_INVALID && installs == 0u);
    uint8_t reserved[MQTT_LEN_MAPPING] = {0x24, 4, 0x20, 0x88, 0x11};
    assert(s1_handle_command(reserved, sizeof reserved, true, install, &ack, &send) == S1_COMMAND_MAPPING);
    assert(ack.result == MQTT_ACK_MALFORMED && installs == 0u);

    /* Not a command: no reply to guess at, nothing installed. */
    uint8_t bad_version[MQTT_LEN_MAPPING] = {0x14, 1, 0x20, 0x88, 0x01};	/* schema v1 */
    assert(s1_handle_command(bad_version, sizeof bad_version, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    assert(s1_handle_command(f, len - 1u, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    uint8_t ack_frame[MQTT_LEN_ACK] = {0x25, 1, 0};
    assert(s1_handle_command(ack_frame, sizeof ack_frame, true, install, &ack, &send) == S1_COMMAND_IGNORED && !send);
    assert(s1_handle_command(f, 0u, true, install, &ack, &send) == S1_COMMAND_IGNORED && installs == 0u);

    /* Snapshot request is served and never acknowledged. */
    uint8_t req[MQTT_LEN_SNAPSHOT_REQUEST];
    mqtt_encode_snapshot_request(req, sizeof req, 5);
    assert(s1_handle_command(req, sizeof req, false, install, &ack, &send) == S1_COMMAND_SNAPSHOT && !send);
}

static bool tx_blocked;
static mqtt_event_t posted_event;

static bool post_event(const uint8_t *frame, unsigned length)
{
    if (tx_blocked)
    {
        return false;
    }
    assert(mqtt_decode_event(frame, length, &posted_event) == MQTT_DECODE_OK);
    return true;
}

static void test_event_backpressure(void)
{
    s1_event_log_t log;
    s1_log_init(&log);
    mqtt_event_t first = {.kind = MQTT_EVENT_HUMP, .arg = 25u,
        .time_ms = 123u, .left_count = 456u, .right_count = 457u};
    mqtt_event_t second = first;
    second.time_ms = 124u;
    assert(s1_log_push(&log, &first) && s1_log_push(&log, &second));
    tx_blocked = true;
    for (unsigned i = 0u; i < 20u; ++i)
    {
        assert(!s1_log_post_oldest(&log, post_event));
        assert(s1_log_count(&log) == 2u && log.dropped == 0u);
    }
    tx_blocked = false;
    assert(s1_log_post_oldest(&log, post_event));
    assert_event_equal(&first, &posted_event);
    assert(s1_log_post_oldest(&log, post_event));
    assert_event_equal(&second, &posted_event);
    assert(!s1_log_post_oldest(&log, post_event));
}

int main(void)
{
    test_event_backpressure();
    test_event_log();
    test_event_packing();
    test_assemble();
    test_mapping_flow();
    puts("s1_report: PASS");
    return 0;
}
