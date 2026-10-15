#ifndef S1_STATUS_PROTOCOL_H
#define S1_STATUS_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define S1_REQUEST_MAX 64u
#define S1_STATUS_MAX 256u
#define S1_HEARTBEAT_PERIOD_MS 1000u

typedef struct
{
    char payload[S1_REQUEST_MAX + 1u];
    unsigned expected, received;
    bool receiving, events, barcode;
    uint32_t after_id;
} S1StatusRequest;

typedef struct
{
    uint32_t uptime_ms, connection_sequence;
    uint32_t rejected_requests, dropped_publish, telemetry_sequence;
    bool have_snapshot;
} S1NetworkStatus;

/* Network-owner-only fragment assembly. No actuation command is represented. */
void s1_status_request_reset(S1StatusRequest *request);
bool s1_status_request_begin(S1StatusRequest *request, uint32_t length);
bool s1_status_request_data(S1StatusRequest *request, const void *data,
                            unsigned length, bool last);
unsigned s1_status_encode(const S1NetworkStatus *status, char *out,
                          unsigned capacity);
unsigned s1_mapping_ack_encode(uint32_t id, unsigned result, uint32_t revision,
                               char *out, unsigned capacity);
#endif
