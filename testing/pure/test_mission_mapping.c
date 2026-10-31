#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include "../../app_program/mission/mission_mapping.h"

enum { EXCHANGES = 100000u };

static void *io_producer(void *unused)
{
    (void)unused;
    for (unsigned i = 0u; i < EXCHANGES; ++i)
    {
        mqtt_mapping_t mapping = {.sequence = (uint8_t)i};
        for (unsigned n = 0u; n < 4u; ++n)
        {
            mapping.letters[n] = (uint8_t)(i % 23u + n);
        }
        while (!mission_mapping_submit(&mapping))
        {
            sched_yield();
        }
        mqtt_ack_t ack;
        while (!mission_mapping_poll(&ack))
        {
            sched_yield();
        }
        assert(ack.sequence == (uint8_t)i);
        assert(ack.result == (i % 2u == 0u ? MQTT_ACK_OK : MQTT_ACK_BUSY));
    }
    return NULL;
}

static void *mission_consumer(void *unused)
{
    (void)unused;
    for (unsigned i = 0u; i < EXCHANGES; ++i)
    {
        mqtt_mapping_t mapping;
        while (!mission_mapping_take(&mapping, NULL))
        {
            sched_yield();
        }
        assert(mapping.sequence == (uint8_t)i);
        for (unsigned n = 0u; n < 4u; ++n)
        {
            assert(mapping.letters[n] == i % 23u + n);
        }
        mission_mapping_reply(i % 2u == 0u ? MQTT_ACK_OK : MQTT_ACK_BUSY);
    }
    return NULL;
}

int main(void)
{
    mqtt_mapping_t request = {.sequence = 255u, .letters = {0u, 1u, 24u, 25u}};
    mqtt_mapping_t taken;
    mqtt_ack_t ack;
    assert(!mission_mapping_poll(&ack));
    assert(!mission_mapping_take(&taken, NULL));
    assert(mission_mapping_submit(&request));
    assert(!mission_mapping_submit(&request));
    assert(!mission_mapping_poll(&ack)); /* receipt cannot acknowledge installation */
    assert(mission_mapping_take(&taken, NULL));
    assert(!mission_mapping_take(&taken, NULL));
    assert(taken.sequence == 255u && taken.letters[0] == 0u && taken.letters[3] == 25u);
    mission_mapping_reply(MQTT_ACK_BUSY); /* Mission started before it took the request */
    assert(!mission_mapping_submit(&request)); /* reply still belongs to I/O */
    assert(mission_mapping_poll(&ack));
    assert(ack.sequence == 255u && ack.result == MQTT_ACK_BUSY);
    assert(!mission_mapping_poll(&ack));
    request.sequence = 0u;
    assert(mission_mapping_submit(&request));
    assert(mission_mapping_take(&taken, NULL));
    mission_mapping_reply(MQTT_ACK_OK);
    assert(mission_mapping_poll(&ack) && ack.sequence == 0u && ack.result == MQTT_ACK_OK);
    request.letters[3] = 0u;
    assert(!mission_mapping_submit(&request));
    request.letters[3] = 26u;
    assert(!mission_mapping_submit(&request));
    uint8_t direction;
    assert(!mission_direction_submit(10u, 0u));
    assert(!mission_direction_submit(10u, 5u));
    assert(mission_direction_submit(10u, 4u));
    assert(!mission_mapping_submit(&request));
    assert(mission_mapping_take(&taken, &direction));
    assert(taken.sequence == 10u && direction == 4u);
    mission_mapping_reply(MQTT_ACK_OK);
    assert(mission_mapping_poll(&ack) && ack.sequence == 10u);
    assert(!mission_direction_submit(10u, 4u));
    assert(mission_direction_submit(11u, 1u));
    assert(mission_mapping_take(&taken, &direction) && direction == 1u);
    mission_mapping_reply(MQTT_ACK_BUSY);
    assert(mission_mapping_poll(&ack) && ack.result == MQTT_ACK_BUSY);
    pthread_t io, mission;
    assert(pthread_create(&io, NULL, io_producer, NULL) == 0);
    assert(pthread_create(&mission, NULL, mission_consumer, NULL) == 0);
    assert(pthread_join(io, NULL) == 0);
    assert(pthread_join(mission, NULL) == 0);
    puts("mission_mapping: 100000 concurrent exchanges passed");
    puts("mission_mapping: PASS (receipt/install/reply ordering, busy and invalid)");
    return 0;
}
