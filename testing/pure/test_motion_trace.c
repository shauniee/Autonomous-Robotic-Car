#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/motion/motion.h"

static mqtt_wheel_trace_t sample(uint32_t time)
{
    return (mqtt_wheel_trace_t){.sampled_ms = time,
        .target_cps = {1000.0f, -1000.0f},
        .measured_cps = {995.0f, -995.0f},
        .duty = {0.8f, -0.8f}, .output_ok = true};
}

static uint32_t time_from(const uint8_t *frame)
{
    uint32_t time = 0u;
    for (unsigned i = 0u; i < 4u; ++i)
    {
        time |= (uint32_t)frame[1u + i] << (8u * i);
    }
    return time;
}

static void *produce(void *unused)
{
    (void)unused;
    for (uint32_t i = 0u; i < 50000u; ++i)
    {
        mqtt_wheel_trace_t trace = sample(i);
        while (!motion_trace_publish(&trace))
        {
            sched_yield();
        }
    }
    return NULL;
}

int main(void)
{
    uint8_t frame[16];
    mqtt_wheel_trace_t trace = sample(0x12345678u);
    const uint8_t golden[] = {0x28, 0x78, 0x56, 0x34, 0x12,
        0xE8, 0x03, 0x0C, 0xBE, 0xE3, 0x03, 0x1D, 0xFC, 0x50, 0xB0};
    memset(frame, 0xAA, sizeof frame);
    assert(mqtt_encode_wheel_trace(frame, 14u, &trace) == 0u);
    assert(frame[0] == 0xAA);
    assert(mqtt_encode_wheel_trace(NULL, 15u, &trace) == 0u);
    assert(mqtt_encode_wheel_trace(frame, 15u, NULL) == 0u);
    assert(mqtt_encode_wheel_trace(frame, 15u, &trace) == 15u);
    assert(memcmp(frame, golden, 15u) == 0 && frame[15] == 0xAA);
    mqtt_type_t type;
    assert(mqtt_frame_type(frame, 15u, &type) == MQTT_DECODE_OK);
    assert(type == MQTT_TYPE_WHEEL_TRACE);
    assert(mqtt_frame_type(frame, 14u, &type) == MQTT_DECODE_LENGTH);
    trace.target_cps[0] = 16383.0f;
    trace.target_cps[1] = -16383.0f;
    trace.measured_cps[0] = 32767.0f;
    trace.measured_cps[1] = -32767.0f;
    assert(mqtt_encode_wheel_trace(frame, 15u, &trace) == 15u);
    assert(frame[5] == 0xFF && frame[6] == 0xBF && frame[7] == 0u && frame[8] == 0xA0);
    assert(frame[9] == 0xFF && frame[10] == 0x7F && frame[11] == 1 && frame[12] == 0x80);
    trace = (mqtt_wheel_trace_t){.target_cps = {NAN, 16384.0f},
        .measured_cps = {INFINITY, -32768.0f}, .inhibited = true};
    mqtt_encode_wheel_trace(frame, 15u, &trace);
    assert(frame[5] == 0 && frame[6] == 0x40 && frame[7] == 0 && frame[8] == 0x60);
    assert(frame[9] == 0 && frame[10] == 0x80 && frame[11] == 0 && frame[12] == 0x80);
    assert(frame[13] == 0x80 && frame[14] == 0x80);
    assert(motion_trace_take(frame, sizeof frame) == 0u);
    assert(!motion_trace_publish(NULL));
    for (unsigned i = 0u; i < 8u; ++i)
    {
        trace = sample(i);
        assert(motion_trace_publish(&trace));
    }
    assert(!motion_trace_publish(&trace));
    assert(motion_trace_take(frame, 14u) == 0u);
    assert(motion_trace_take(NULL, 15u) == 0u);
    for (unsigned i = 0u; i < 8u; ++i)
    {
        assert(motion_trace_take(frame, sizeof frame) == 15u);
        assert(time_from(frame) == i);
    }
    pthread_t producer;
    assert(pthread_create(&producer, NULL, produce, NULL) == 0);
    for (uint32_t i = 0u; i < 50000u; ++i)
    {
        while (motion_trace_take(frame, sizeof frame) == 0u)
        {
            sched_yield();
        }
        assert(time_from(frame) == i);
        assert(memcmp(&frame[5], &golden[5], 10u) == 0);
    }
    assert(pthread_join(producer, NULL) == 0);
    puts("motion_trace: PASS (golden, bounds, invalid, full/empty, 50000 concurrent records)");
    return 0;
}
