#define PROHIBIT_DEF_SIZE_T
/* S1 reporting transport; every call runs in the existing core-0 service. */
#include "mqtt_utk.h"
#include "../../../../app_program/subsystem1/mapping_ack.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#include "cyw43.h"
#include "wifi_secrets.h"
#include "../../../../app_program/subsystem1/telemetry.h"
#include "../../../../app_program/subsystem1/status_protocol.h"
#include <tm/tmonitor.h>
#include <string.h>
#include "../../../../app_program/subsystem1/mapping_protocol.h"
#include "../../../../app_program/subsystem3/mission_status.h"
#ifndef MQTT_MAPPING_TOPIC
#define MQTT_MAPPING_TOPIC "robot/s3/mapping"
#endif
#ifndef MQTT_MAPPING_STATUS_TOPIC
#define MQTT_MAPPING_STATUS_TOPIC "robot/s3/mapping_status"
#endif

#ifndef MQTT_EVENTS_TOPIC
#define MQTT_EVENTS_TOPIC "robot/s3/events"
#endif

/* Defaults also support existing ignored wifi_secrets.h files. */
#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif
#ifndef MQTT_HEARTBEAT_TOPIC
#define MQTT_HEARTBEAT_TOPIC "robot/s3/heartbeat"
#endif
#ifndef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC "robot/s3/status"
#endif
#ifndef MQTT_REQUEST_TOPIC
#define MQTT_REQUEST_TOPIC "robot/s3/request"
#endif

#define MQTT_KEEPALIVE_S 10u
#define MQTT_ATTEMPT_MS 15000u
#define MQTT_RETRY_MIN_MS 1000u
#define MQTT_RETRY_MAX_MS 30000u
#define MQTT_TOPIC_MAX 96u

static mqtt_client_t *client;
static ip_addr_t broker_addr;
static char client_id[64];
static bool link_ready, active, online, retry_pending, bad_configuration;
static bool status_pending;
static uint32_t retry_ms = MQTT_RETRY_MIN_MS;
static uint32_t heartbeat_ms, request_ms;
static bool have_request;
static S1StatusRequest request;
static S1MappingRequest mapping_request;
static bool receiving_mapping;
static S1AckQueue acknowledgements;
static bool events_pending, events_barcode;
static uint32_t events_after;
static S1NetworkStatus diagnostics;
static void publish_timeout(void *arg);
static void retry_timeout(void *arg);
static void attempt_timeout(void *arg);

static void cancel_timers(void)
{
    sys_untimeout(publish_timeout, NULL);
    sys_untimeout(retry_timeout, NULL);
    sys_untimeout(attempt_timeout, NULL);
}

static void schedule_retry(void)
{
    cancel_timers();
    online = false;
    active = false;
    status_pending = false;
    s1_status_request_reset(&request);
    s1_mapping_reset(&mapping_request);
    receiving_mapping = false;
    events_pending = false;
    s1_ack_reconnect(&acknowledgements);
    /* Close failed attempts, including TCP attempts that never completed. */
    if (client)
    {
        mqtt_disconnect(client);
    }
    retry_pending = true;
    sys_timeout(retry_ms, retry_timeout, NULL);
    if (retry_ms < MQTT_RETRY_MAX_MS)
    {
        retry_ms *= 2u;
        if (retry_ms > MQTT_RETRY_MAX_MS)
        {
            retry_ms = MQTT_RETRY_MAX_MS;
        }
    }
}

static void publish_done(void *arg, err_t result)
{
    (void)arg;
    if (result != ERR_OK)
    {
        ++diagnostics.dropped_publish;
    }
}

static void publish(const char *topic, const char *payload, unsigned length,
                    bool retained)
{
    if (length == 0u)
    {
        return;
    }
    err_t result =
        mqtt_publish(client, topic, payload, (u16_t)length, retained ? 1u : 0u,
                     retained ? 1u : 0u, publish_done, NULL);
    if (result != ERR_OK)
    {
        ++diagnostics.dropped_publish;
    }
}

static void publish_latest(void)
{
    S1Telemetry snapshot;
    char payload[S1_TELEMETRY_PAYLOAD_MAX + 1u];
    diagnostics.have_snapshot = s1_telemetry_copy(&snapshot);
    if (!diagnostics.have_snapshot)
    {
        return;
    }
    diagnostics.telemetry_sequence = snapshot.sequence;
    unsigned length =
        s1_telemetry_encode(&snapshot, sys_now(), payload, sizeof(payload));
    if (length == 0u)
    {
        ++diagnostics.dropped_publish;
        return;
    }
    publish(MQTT_TOPIC, payload, length, false);
}

static void publish_status(bool retained)
{
    char payload[S1_STATUS_MAX + 1u];
    diagnostics.uptime_ms = sys_now();
    unsigned length = s1_status_encode(&diagnostics, payload, sizeof(payload));
    publish(retained ? MQTT_STATUS_TOPIC : MQTT_HEARTBEAT_TOPIC, payload,
            length, retained);
}

static void acknowledgement_done(void *arg, err_t result)
{
    s1_ack_complete(&acknowledgements, (uint32_t)(uintptr_t)arg,
                    result == ERR_OK);
    if (result != ERR_OK)
    {
        ++diagnostics.dropped_publish;
    }
}
static void publish_acknowledgements(void)
{
    MissionSnapshot mission;
    bool have_mission = mission_copy(&mission);
    for (unsigned i = 0u; i < S1_ACK_CAPACITY; ++i)
    {
        S1Ack *ack = &acknowledgements.slots[i];
        if (ack->state == ACK_RESERVED && have_mission &&
            mission.mapping_request_id == ack->id &&
            mission.mapping_result != MAP_PENDING &&
            mission.mapping_result != MAP_NONE)
        {
            s1_ack_ready(&acknowledgements, i, mission.mapping_result,
                         mission.mapping_revision);
        }
        if (ack->state == ACK_READY)
        {
            char payload[128];
            unsigned length = s1_mapping_ack_encode(
                ack->id, ack->result, ack->revision, payload, sizeof(payload));
            if (length != 0u)
            {
                uint32_t token = s1_ack_send(&acknowledgements, i);
                if (token != 0u &&
                    mqtt_publish(client, MQTT_MAPPING_STATUS_TOPIC, payload,
                                 (u16_t)length, 1u, 0u, acknowledgement_done,
                                 (void *)(uintptr_t)token) != ERR_OK)
                {
                    s1_ack_complete(&acknowledgements, token, false);
                    ++diagnostics.dropped_publish;
                }
            }
        }
    }
}
static void publish_timeout(void *arg)
{
    (void)arg;
    if (!online || !link_ready || !mqtt_client_is_connected(client))
    {
        return;
    }
    publish_acknowledgements();
    publish_latest();
    uint32_t now = sys_now();
    if ((uint32_t)(now - heartbeat_ms) >= S1_HEARTBEAT_PERIOD_MS)
    {
        heartbeat_ms = now;
        publish_status(false);
    }
    if (status_pending || events_pending)
    {
        /* Static storage belongs solely to the kernel network owner. */
        static S1Telemetry snapshot;
        static char payload[S1_TELEMETRY_PAYLOAD_MAX + 1u];
        if (s1_telemetry_copy(&snapshot))
        {
            unsigned length =
                events_pending
                    ? s1_events_encode(events_barcode ? &snapshot.barcodes
                                                      : &snapshot.humps,
                                       events_barcode, events_after, payload,
                                       sizeof(payload))
                    : s1_robot_status_encode(&snapshot, now, payload,
                                             sizeof(payload));
            publish(events_pending ? MQTT_EVENTS_TOPIC : MQTT_STATUS_TOPIC,
                    payload, length, false);
        }
        status_pending = events_pending = false;
    }
    sys_timeout(S1_TELEMETRY_PERIOD_MS, publish_timeout, NULL);
}

static void retry_timeout(void *arg)
{
    (void)arg;
    retry_pending = false;
    mqtt_utk_start(); /* The link guard prevents broker attempts while offline.
                       */
}

static void attempt_timeout(void *arg)
{
    (void)arg;
    tm_printf(
        (UB *)"mqtt: connection/subscription deadline; retry scheduled\n");
    schedule_retry();
}

static void incoming_publish(void *arg, const char *topic, u32_t length)
{
    (void)arg;
    s1_status_request_reset(&request);
    s1_mapping_reset(&mapping_request);
    receiving_mapping = false;
    if (online && strcmp(topic, MQTT_MAPPING_TOPIC) == 0)
    {
        receiving_mapping = s1_mapping_begin(&mapping_request, length);
        if (!receiving_mapping)
        {
            ++diagnostics.rejected_requests;
        }
        return;
    }
    if (!online || (strcmp(topic, MQTT_REQUEST_TOPIC) != 0))
    {
        ++diagnostics.rejected_requests;
        return;
    }
    if (!s1_status_request_begin(&request, length))
    {
        ++diagnostics.rejected_requests;
    }
}

static void incoming_data(void *arg, const u8_t *data, u16_t length, u8_t flags)
{
    (void)arg;
    if (receiving_mapping)
    {
        bool last = (flags & MQTT_DATA_FLAG_LAST) != 0u;
        if (!s1_mapping_data(&mapping_request, data, length, last))
        {
            receiving_mapping = false;
            ++diagnostics.rejected_requests;
            return;
        }
        if (last)
        {
            receiving_mapping = false;
            bool pending = s1_ack_reserved(&acknowledgements);
            int slot =
                s1_ack_reserve(&acknowledgements, mapping_request.request_id);
            if (slot < 0)
            {
                ++diagnostics.rejected_requests;
                return; /* Full result storage never applies configuration. */
            }
            MappingResult result =
                pending ? MAP_BUSY
                        : mission_mapping_submit(mapping_request.request_id,
                                                 mapping_request.mapping);
            if (result != MAP_PENDING)
            {
                s1_ack_ready(&acknowledgements, (unsigned)slot, result, 0u);
            }
        }
        return;
    }
    if (!request.receiving)
    {
        return;
    }
    bool last = (flags & MQTT_DATA_FLAG_LAST) != 0u;
    if (!s1_status_request_data(&request, data, length, last))
    {
        ++diagnostics.rejected_requests;
        return;
    }
    if (last)
    {
        uint32_t now = sys_now();
        if (have_request &&
            ((uint32_t)(now - request_ms) < S1_HEARTBEAT_PERIOD_MS))
        {
            ++diagnostics.rejected_requests;
            return;
        }
        request_ms = now;
        have_request = true;
        events_pending = request.events;
        status_pending = !request.events;
        events_barcode = request.barcode;
        events_after = request.after_id;
    }
}

static void subscribed(void *arg, err_t result)
{
    (void)arg;
    if (!active || !link_ready)
    {
        return;
    }
    if (result != ERR_OK)
    {
        schedule_retry();
        return;
    }
    sys_untimeout(attempt_timeout, NULL);
    online = true;
    retry_ms = MQTT_RETRY_MIN_MS;
    have_request = false;
    ++diagnostics.connection_sequence;
    publish_latest();
    publish_status(true);
    publish_status(false);
    heartbeat_ms = sys_now();
    sys_timeout(S1_TELEMETRY_PERIOD_MS, publish_timeout, NULL);
    tm_printf((UB *)"mqtt: telemetry and status subscription ready\n");
}

static void mapping_subscribed(void *arg, err_t result)
{
    if (result != ERR_OK)
    {
        schedule_retry();
        return;
    }
    if (mqtt_subscribe(client, MQTT_MAPPING_TOPIC, 1u, subscribed, arg) !=
        ERR_OK)
    {
        schedule_retry();
    }
}

static void connection_cb(mqtt_client_t *c, void *arg,
                          mqtt_connection_status_t status)
{
    (void)c;
    (void)arg;
    if ((status != MQTT_CONNECT_ACCEPTED) || !link_ready)
    {
        schedule_retry();
        return;
    }
    mqtt_set_inpub_callback(client, incoming_publish, incoming_data, NULL);
    if (mqtt_subscribe(client, MQTT_REQUEST_TOPIC, 1u, mapping_subscribed,
                       NULL) != ERR_OK)
    {
        schedule_retry();
    }
}

static bool configure(void)
{
    const char *topics[] = {MQTT_TOPIC,         MQTT_HEARTBEAT_TOPIC,
                            MQTT_STATUS_TOPIC,  MQTT_REQUEST_TOPIC,
                            MQTT_MAPPING_TOPIC, MQTT_MAPPING_STATUS_TOPIC,
                            MQTT_EVENTS_TOPIC};
    if (!ip4addr_aton(MQTT_BROKER_IP, ip_2_ip4(&broker_addr)) ||
        ip4_addr_isany(ip_2_ip4(&broker_addr)) || (MQTT_BROKER_PORT < 1) ||
        (MQTT_BROKER_PORT > 65535) || (MQTT_CLIENT_ID[0] == '\0') ||
        (strlen(MQTT_CLIENT_ID) > 40u))
    {
        return false;
    }
    if (!s1_topics_valid(topics, sizeof(topics) / sizeof(topics[0])))
    {
        return false;
    }
    uint8_t mac[6];
    if (cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac) != 0)
    {
        return false;
    }
    static const char hex[] = "0123456789abcdef";
    unsigned length = (unsigned)strlen(MQTT_CLIENT_ID);
    memcpy(client_id, MQTT_CLIENT_ID, length);
    client_id[length++] = '-';
    for (unsigned i = 0u; i < sizeof(mac); ++i)
    {
        client_id[length++] = hex[mac[i] >> 4u];
        client_id[length++] = hex[mac[i] & 0x0fu];
    }
    client_id[length] = '\0';
    IP_SET_TYPE(&broker_addr, IPADDR_TYPE_V4);
    return true;
}

void mqtt_utk_start(void)
{
    if (!link_ready || bad_configuration || active || retry_pending)
    {
        return;
    }
    if (!configure())
    {
        bad_configuration = true;
        tm_printf((UB *)"mqtt: invalid broker/client/topic configuration\n");
        return;
    }
    if (!client)
    {
        client = mqtt_client_new();
        if (!client)
        {
            schedule_retry();
            return;
        }
    }
    struct mqtt_connect_client_info_t info = {0};
    info.client_id = client_id;
    info.client_user = MQTT_USERNAME[0] ? MQTT_USERNAME : NULL;
    info.client_pass =
        (MQTT_USERNAME[0] && MQTT_PASSWORD[0]) ? MQTT_PASSWORD : NULL;
    info.keep_alive = MQTT_KEEPALIVE_S;
    info.will_topic = MQTT_STATUS_TOPIC;
    info.will_msg = "{\"schema\":1,\"online\":false}";
    info.will_qos = 1u;
    info.will_retain = 1u;
    active = true;
    sys_timeout(MQTT_ATTEMPT_MS, attempt_timeout, NULL);
    if (mqtt_client_connect(client, &broker_addr, MQTT_BROKER_PORT,
                            connection_cb, NULL, &info) != ERR_OK)
    {
        schedule_retry();
    }
}

void mqtt_utk_poll(bool link_up)
{
    bool lost = link_ready && !link_up;
    link_ready = link_up;
    if (lost && active)
    {
        schedule_retry(); /* Discard fragments; never call a motion interface.
                           */
    }
    mqtt_utk_start();
}
