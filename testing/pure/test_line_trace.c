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
    mqtt_line_trace_t level = sample(0u);
    level.flags = MQTT_LINE_BARCODE_LEVEL;
    assert(mqtt_encode_line_trace(frame, 17u, &level) == 17u && frame[5] == 0x80u);
    const uint16_t runs[MQTT_BARCODE_TRACE_RUNS] = {0x8022u, 12u};
    assert(mqtt_encode_barcode_trace(frame, 17u, 7u, runs) == 0u);
    uint8_t bars[MQTT_LEN_BARCODE_TRACE];
    assert(mqtt_encode_barcode_trace(bars, sizeof bars, 7u, runs) == 18u);
    assert(bars[0] == 0x2Au && bars[1] == 7u && bars[2] == 0x22u && bars[3] == 0x80u && bars[4] == 12u);
    mqtt_type_t bars_type;
    assert(mqtt_frame_type(bars, 18u, &bars_type) == MQTT_DECODE_OK && bars_type == MQTT_TYPE_BARCODE_TRACE);
    uint8_t hump[MQTT_LEN_TERRAIN_TRACE];
    assert(mqtt_encode_terrain_trace(hump, sizeof hump, 5u, 14, 0xF9u, 3u, 100u, -6) == 8u);
    assert(hump[0] == 0x2Bu && hump[1] == 5u && hump[3] == 14u && hump[4] == 0xF9u && hump[6] == 100u && hump[7] == 0xFAu);
    mqtt_line_trace_t edges = sample(0u);
    edges.barcode_edges = 31u;
    assert(mqtt_encode_line_trace(frame, 17u, &edges) == 17u && (frame[15] >> 3) == 31u);
    edges.barcode_edges = 32u;
    assert(mqtt_encode_line_trace(frame, 17u, &edges) == 0u);
    mqtt_line_trace_t bad[] = {sample(0u), sample(0u), sample(0u), sample(0u)};
    bad[0].mission_state = 16u;
    bad[1].phase = 8u;
    bad[2].command_mode = 4u;
    bad[3].command_speed = 512u;
    for (unsigned i = 0u; i < 4u; ++i)
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
