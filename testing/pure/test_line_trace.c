#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/mission/line_trace.h"

/* Same bytes as golden_line in scripts/mqtt_client.py --self-test. */
static const uint8_t golden[] = {0x29, 0x78, 0x56, 0x34, 0x12, 0x13, 0x22,
    0x34, 0x12, 0x78, 0x56, 0xE0, 0xFE, 0xE0, 0x64, 0x02, 0x0C};

static mqtt_line_trace_t sample(uint32_t time)
{
    return (mqtt_line_trace_t){.sampled_ms = time,
        .flags = MQTT_LINE_LEFT_BLACK | MQTT_LINE_RIGHT_BLACK | MQTT_LINE_JUNCTION_QUALIFIED,
        .mission_state = 2u, .phase = 2u, .wheel_count = {0x1234u, 0x5678u},
        .steer_q = -32, .law_q = -2, .command_steer_q = -32,
        .command_mode = 1u, .command_speed = 100u, .onset_skew_mm = 12u};
}

int main(void)
{
    uint8_t frame[18];
    mqtt_line_trace_t trace = sample(0x12345678u);
    memset(frame, 0xAA, sizeof frame);
    assert(mqtt_encode_line_trace(frame, 16u, &trace) == 0u && frame[0] == 0xAA);
    assert(mqtt_encode_line_trace(NULL, 17u, &trace) == 0u);
    assert(mqtt_encode_line_trace(frame, 17u, NULL) == 0u);
    assert(mqtt_encode_line_trace(frame, 17u, &trace) == 17u);
    assert(memcmp(frame, golden, 17u) == 0 && frame[17] == 0xAA);
    mqtt_type_t type;
    assert(mqtt_frame_type(frame, 17u, &type) == MQTT_DECODE_OK && type == MQTT_TYPE_LINE_TRACE);
    assert(mqtt_frame_type(frame, 16u, &type) == MQTT_DECODE_LENGTH);
    mqtt_line_trace_t bad[] = {sample(0u), sample(0u), sample(0u), sample(0u), sample(0u)};
    bad[0].flags = 0x80u;
    bad[1].mission_state = 16u;
    bad[2].phase = 8u;
    bad[3].command_mode = 4u;
    bad[4].command_speed = 512u;
    for (unsigned i = 0u; i < 5u; ++i)
    {
        assert(mqtt_encode_line_trace(frame, 17u, &bad[i]) == 0u);
    }

    assert(line_trace_take(frame, sizeof frame) == 0u);
    assert(!line_trace_publish(NULL) && !line_trace_publish(&bad[0]));
    for (unsigned i = 0u; i < 8u; ++i)
    {
        trace = sample(i);
        assert(line_trace_publish(&trace));
    }
    assert(!line_trace_publish(&trace)); /* full drops the newest */
    assert(line_trace_take(frame, 16u) == 0u && line_trace_take(NULL, 17u) == 0u);
    for (unsigned i = 0u; i < 8u; ++i)
    {
        assert(line_trace_take(frame, sizeof frame) == 17u);
        assert(frame[1] == i && memcmp(&frame[5], &golden[5], 12u) == 0);
    }
    assert(line_trace_take(frame, sizeof frame) == 0u);
    puts("line_trace: PASS (golden, bounds, invalid fields, full/empty order)");
    return 0;
}
