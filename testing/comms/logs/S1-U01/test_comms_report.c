/* Host test for app_program/comms/comms_report.[ch] (issues #41, #43 logic). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../../app_program/comms/comms.h"

static mqtt_event_t ev(uint8_t arg) { return (mqtt_event_t){.kind = MQTT_EVENT_HUMP, .arg = arg}; }

static void test_event_log(void)
{
    comms_event_log_t log; mqtt_event_t e;
    comms_log_init(&log);
    assert(!comms_log_pop(&log, &e));
    for (uint8_t i = 0; i < COMMS_EVENT_CAPACITY; ++i) { mqtt_event_t x = ev(i); comms_log_push(&log, &x); }
    assert(comms_log_count(&log) == COMMS_EVENT_CAPACITY);
    /* Overflow: the oldest events go, the newest are kept. */
    for (uint8_t i = 0; i < 5u; ++i) { mqtt_event_t x = ev((uint8_t)(100u + i)); comms_log_push(&log, &x); }
    assert(comms_log_count(&log) == COMMS_EVENT_CAPACITY);
    assert(comms_log_pop(&log, &e) && e.arg == 5u);		/* events 0-4 were overwritten */
    unsigned n = 1u; uint8_t last = e.arg;
    while (comms_log_pop(&log, &e)) { ++n; last = e.arg; }
    assert(n == COMMS_EVENT_CAPACITY && last == 104u && comms_log_count(&log) == 0u);
    /* Wrap of the ring indices: push/pop beyond capacity keeps order. */
    for (uint8_t i = 0; i < 3u * COMMS_EVENT_CAPACITY; ++i) {
        mqtt_event_t x = ev(i); comms_log_push(&log, &x);
        assert(comms_log_pop(&log, &e) && e.arg == i);
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
       The 16-bit obstacle clearance must survive, even though barcode action
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
    comms_event_log_t log;
    comms_log_init(&log);
    for (unsigned i = 0; i < sizeof samples / sizeof samples[0]; ++i)
    {
        mqtt_event_t roundtrip;
        uint8_t before[MQTT_LEN_EVENT], after[MQTT_LEN_EVENT];
        assert(mqtt_encode_event(before, sizeof before, &samples[i]) == MQTT_LEN_EVENT);
        assert(comms_log_push(&log, &samples[i]));
        assert(comms_log_pop(&log, &roundtrip));
        assert_event_equal(&samples[i], &roundtrip);
        assert(mqtt_encode_event(after, sizeof after, &roundtrip) == MQTT_LEN_EVENT);
        assert(memcmp(before, after, MQTT_LEN_EVENT) == 0); /* schema v2 unchanged */
    }
    assert(comms_log_count(&log) == 0u);

    /* Reject out-of-range inputs rather than silently dropping the high bit.
       A rejected event must not disturb order, count or drop statistics. */
    mqtt_event_t bad = {.kind = MQTT_EVENT_HUMP, .arg = 128};
    assert(!comms_log_push(&log, &bad));
    bad.kind = 0;
    bad.arg = 0;
    assert(!comms_log_push(&log, &bad));
    bad.kind = MQTT_EVENT_COUNT;
    assert(!comms_log_push(&log, &bad));
    assert(comms_log_count(&log) == 0u);

    /* Full-ring overwrite keeps the newest event. */
    for (unsigned i = 0; i < COMMS_EVENT_CAPACITY; ++i)
    {
        mqtt_event_t x = ev((uint8_t)i);
        assert(comms_log_push(&log, &x));
    }
    mqtt_event_t newest = ev(42);
    assert(comms_log_push(&log, &newest));
    assert(comms_log_count(&log) == COMMS_EVENT_CAPACITY);
    mqtt_event_t popped;
    assert(comms_log_pop(&log, &popped) && popped.arg == 1u);
}

static void test_assemble(void)
{
    mqtt_telemetry_t t = {.sequence = 9, .mission_state = 40, .motion_status = 9, .motion_action = 200,
                          .line_status = 9, .hump_max = 255, .front_mm = 60000, .front_valid = true,
                          .barcode_letter = 26, .barcode_action = 12, .faults = {1, 2, 3, 4, 5, 6},
                          .left_count = 0xABCD, .hump_count = 7, .hump_valid = true, .obstacle_width_mm = 65535};
    uint8_t frame[MQTT_FRAME_MAX];
    comms_clamp(&t);
    assert(t.sequence == 9 && t.mission_state == 15 && t.motion_status == 3 && t.motion_action == 63 && t.line_status == 3);
    assert(t.hump_max == 127 && t.front_mm == 8191 && t.barcode_letter == MQTT_NO_LETTER && t.barcode_action == 0);
    assert(t.faults[5] == 6 && t.left_count == 0xABCD && t.hump_count == 7 && !t.hump_valid && !t.front_valid && t.obstacle_width_mm == 65535);
    assert(mqtt_encode_telemetry(frame, sizeof frame, MQTT_TYPE_TELEMETRY, &t) == MQTT_LEN_TELEMETRY);	/* always encodable */
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
    comms_event_log_t log;
    comms_log_init(&log);
    mqtt_event_t first = {.kind = MQTT_EVENT_HUMP, .arg = 25u,
        .time_ms = 123u, .left_count = 456u, .right_count = 457u};
    mqtt_event_t second = first;
    second.time_ms = 124u;
    assert(comms_log_push(&log, &first) && comms_log_push(&log, &second));
    tx_blocked = true;
    for (unsigned i = 0u; i < 20u; ++i)
    {
        assert(!comms_log_post_oldest(&log, post_event));
        assert(comms_log_count(&log) == 2u);
    }
    tx_blocked = false;
    assert(comms_log_post_oldest(&log, post_event));
    assert_event_equal(&first, &posted_event);
    assert(comms_log_post_oldest(&log, post_event));
    assert_event_equal(&second, &posted_event);
    assert(!comms_log_post_oldest(&log, post_event));
}

int main(void)
{
    test_event_backpressure();
    test_event_log();
    test_event_packing();
    test_assemble();
    puts("comms_report: PASS");
    return 0;
}
