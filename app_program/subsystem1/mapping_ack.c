#include "mapping_ack.h"
#include <string.h>
int s1_ack_reserve(S1AckQueue *q, uint32_t id)
{
    if (id == 0u)
    {
        return -1;
    }
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        if (q->slots[i].state != ACK_EMPTY && q->slots[i].id == id)
        {
            return -1;
        }
    }
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        if (q->slots[i].state == ACK_EMPTY)
        {
            q->slots[i] = (S1Ack){.id = id, .state = ACK_RESERVED};
            return (int)i;
        }
    }
    return -1;
}
void s1_ack_ready(S1AckQueue *q, unsigned i, unsigned result, uint32_t revision)
{
    if (i < S1_ACK_CAPACITY && q->slots[i].state == ACK_RESERVED)
    {
        q->slots[i].result = result;
        q->slots[i].revision = revision;
        q->slots[i].state = ACK_READY;
    }
}
bool s1_ack_reserved(const S1AckQueue *q)
{
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        if (q->slots[i].state == ACK_RESERVED)
        {
            return true;
        }
    }
    return false;
}
uint32_t s1_ack_send(S1AckQueue *q, unsigned i)
{
    if (i >= S1_ACK_CAPACITY || q->slots[i].state != ACK_READY)
    {
        return 0u;
    }
    /* At most four tokens are live; skip zero and live wrap collisions. */
    for (unsigned attempt = 0u; attempt <= S1_ACK_CAPACITY; ++attempt)
    {
        if (++q->serial == 0u)
        {
            ++q->serial;
        }
        bool used = false;
        for (unsigned j = 0u; j < S1_ACK_CAPACITY; ++j)
        {
            used = used || q->slots[j].token == q->serial;
        }
        if (!used)
        {
            q->slots[i].token = q->serial;
            q->slots[i].state = ACK_FLIGHT;
            return q->serial;
        }
    }
    return 0u;
}
void s1_ack_complete(S1AckQueue *q, uint32_t token, bool delivered)
{
    if (token == 0u)
    {
        return;
    }
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        if (q->slots[i].state == ACK_FLIGHT && q->slots[i].token == token)
        {
            if (delivered)
            {
                q->slots[i] = (S1Ack){0};
            }
            else
            {
                q->slots[i].state = ACK_READY;
                q->slots[i].token = 0u;
            }
        }
    }
}
void s1_ack_reconnect(S1AckQueue *q)
{
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        if (q->slots[i].state == ACK_FLIGHT)
        {
            q->slots[i].state = ACK_READY;
            q->slots[i].token = 0u;
        }
    }
}
bool s1_topics_valid(const char *const *topics, unsigned count)
{
    if (topics == NULL || count == 0u)
    {
        return false;
    }
    for (unsigned i = 0u; i < count; ++i)
    {
        if (topics[i] == NULL || topics[i][0] == '\0' ||
            strlen(topics[i]) >= 96u || strchr(topics[i], '+') ||
            strchr(topics[i], '#'))
        {
            return false;
        }
        for (unsigned j = 0u; j < i; ++j)
        {
            if (strcmp(topics[i], topics[j]) == 0)
            {
                return false;
            }
        }
    }
    return true;
}
