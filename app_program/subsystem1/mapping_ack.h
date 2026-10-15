#ifndef MAPPING_ACK_H
#define MAPPING_ACK_H
#include <stdbool.h>
#include <stdint.h>
#define S1_ACK_CAPACITY 4u
typedef enum
{
    ACK_EMPTY,
    ACK_RESERVED,
    ACK_READY,
    ACK_FLIGHT
} S1AckState;
typedef struct
{
    uint32_t id, revision, token;
    unsigned result;
    S1AckState state;
} S1Ack;
typedef struct
{
    S1Ack slots[S1_ACK_CAPACITY];
    uint32_t serial;
} S1AckQueue;
int s1_ack_reserve(S1AckQueue *queue, uint32_t id);
void s1_ack_ready(S1AckQueue *queue, unsigned slot, unsigned result,
                  uint32_t revision);
bool s1_ack_reserved(const S1AckQueue *queue);
uint32_t s1_ack_send(S1AckQueue *queue, unsigned slot);
void s1_ack_complete(S1AckQueue *queue, uint32_t token, bool delivered);
void s1_ack_reconnect(S1AckQueue *queue);
bool s1_topics_valid(const char *const *topics, unsigned count);
#endif
