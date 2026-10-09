#include "status_protocol.h"
#include <string.h>

void s1_status_request_reset(S1StatusRequest *request)
{
    if (request)
    {
        memset(request, 0, sizeof(*request));
    }
}

bool s1_status_request_begin(S1StatusRequest *request, uint32_t length)
{
    if (!request)
    {
        return false;
    }
    s1_status_request_reset(request);
    if ((length == 0u) || (length > S1_REQUEST_MAX))
    {
        return false;
    }
    request->expected = (unsigned)length;
    request->receiving = true;
    return true;
}

static void skip_space(const char **cursor)
{
    while ((**cursor == ' ') || (**cursor == '\t') ||
           (**cursor == '\r') || (**cursor == '\n'))
    {
        ++(*cursor);
    }
}

static bool token(const char **cursor, const char *expected)
{
    skip_space(cursor);
    unsigned length = (unsigned)strlen(expected);
    if (strncmp(*cursor, expected, length) != 0)
    {
        return false;
    }
    *cursor += length;
    return true;
}

bool s1_status_request_data(S1StatusRequest *request, const void *data,
                            unsigned length, bool last)
{
    if (!request || !request->receiving)
    {
        return false;
    }
    if ((!data && (length != 0u)) ||
        (length > request->expected - request->received))
    {
        request->receiving = false;
        return false;
    }
    if (length != 0u)
    {
        memcpy(request->payload + request->received, data, length);
    }
    request->received += length;
    if (!last)
    {
        return true; /* Accepted fragment; completion is checked only on LAST. */
    }
    request->receiving = false;
    if ((request->received != request->expected) ||
        memchr(request->payload, '\0', request->received))
    {
        return false;
    }
    request->payload[request->received] = '\0';
    const char *cursor = request->payload;
    if (!token(&cursor, "{") || !token(&cursor, "\"request\"") ||
        !token(&cursor, ":") || !token(&cursor, "\"status\"") ||
        !token(&cursor, "}"))
    {
        return false;
    }
    skip_space(&cursor);
    return *cursor == '\0';
}

typedef struct
{
    char *out;
    unsigned used, capacity;
    bool valid;
} StatusWriter;

static void append_text(StatusWriter *writer, const char *text)
{
    unsigned length = (unsigned)strlen(text);
    if (!writer->valid || (length >= writer->capacity - writer->used))
    {
        writer->valid = false;
        return;
    }
    memcpy(writer->out + writer->used, text, length);
    writer->used += length;
}

static void append_number(StatusWriter *writer, uint32_t value)
{
    char text[11];
    unsigned index = sizeof(text) - 1u;
    text[index] = '\0';
    do
    {
        text[--index] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);
    append_text(writer, text + index);
}

unsigned s1_status_encode(const S1NetworkStatus *status, char *out,
                          unsigned capacity)
{
    if (!out || (capacity == 0u))
    {
        return 0u;
    }
    out[0] = '\0';
    if (!status)
    {
        return 0u;
    }
    if (capacity > S1_STATUS_MAX + 1u)
    {
        capacity = S1_STATUS_MAX + 1u;
    }
    StatusWriter writer = {out, 0u, capacity, true};
    append_text(&writer, "{\"schema\":1,\"online\":true,\"uptime_ms\":");
    append_number(&writer, status->uptime_ms);
    append_text(&writer, ",\"connection_sequence\":");
    append_number(&writer, status->connection_sequence);
    append_text(&writer, ",\"rejected_requests\":");
    append_number(&writer, status->rejected_requests);
    append_text(&writer, ",\"dropped_publish\":");
    append_number(&writer, status->dropped_publish);
    append_text(&writer, ",\"have_snapshot\":");
    append_text(&writer, status->have_snapshot ? "true" : "false");
    append_text(&writer, ",\"telemetry_sequence\":");
    append_number(&writer, status->telemetry_sequence);
    append_text(&writer, "}");
    if (!writer.valid)
    {
        out[0] = '\0';
        return 0u;
    }
    out[writer.used] = '\0';
    return writer.used;
}
