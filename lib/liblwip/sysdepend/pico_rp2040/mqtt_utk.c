/* S1 transport: connect, reconnect, publish opaque byte frames and hand
   received frames to Reporting. It knows no frame layout. Every call runs in
   the existing core-0 radio service. Three topics: telemetry, command and
   status (retained, with a last will). */
#include "mqtt_utk.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#include "cyw43.h"
#include "wifi_secrets.h"
#include "../../../../app_program/comms/comms_bridge.h"
#include <tm/tmonitor.h>
#include <string.h>

#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif
#ifndef MQTT_STATUS_TOPIC
#define MQTT_STATUS_TOPIC "robot/s3/status"
#endif
#ifndef MQTT_COMMAND_TOPIC
#define MQTT_COMMAND_TOPIC "robot/s3/request"
#endif

#define MQTT_KEEPALIVE_S 10u
#define MQTT_ATTEMPT_MS 15000u
#define MQTT_RETRY_MIN_MS 1000u
#define MQTT_RETRY_MAX_MS 30000u
#define MQTT_TOPIC_MAX 96u
#define MQTT_FRAMES_PER_POLL 2u
#define STATUS_ONLINE "1"
#define STATUS_OFFLINE "0"

static mqtt_client_t *client;
static ip_addr_t broker_addr;
static char client_id[64];
static bool link_ready, connecting, online, retry_pending, bad_configuration, in_command_topic;
static uint32_t retry_ms = MQTT_RETRY_MIN_MS;
static uint8_t rx[COMMS_FRAME_MAX];
static unsigned rx_length;
static bool rx_overflow;

static void retry_timeout(void *arg);
static void attempt_timeout(void *arg);

static void cancel_timers(void)
{
    sys_untimeout(retry_timeout, NULL);
    sys_untimeout(attempt_timeout, NULL);
    retry_pending = false;
}

static void set_online(bool value)
{
    online = value;
    comms_bridge_link(link_ready, online);
}

static void schedule_retry(void)
{
    if (retry_pending || bad_configuration)
    {
        return;
    }
    retry_pending = true;
    sys_timeout(retry_ms, retry_timeout, NULL);
    retry_ms = retry_ms >= MQTT_RETRY_MAX_MS / 2u ? MQTT_RETRY_MAX_MS : retry_ms * 2u;	/* bounded backoff */
}

static void drop_connection(void)
{
    connecting = false;
    set_online(false);
    if (client != NULL && mqtt_client_is_connected(client))
    {
        mqtt_disconnect(client);
    }
    sys_untimeout(attempt_timeout, NULL);
}

static void publish_status(const char *payload)
{
    (void)mqtt_publish(client, MQTT_STATUS_TOPIC, payload, 1u, 1u, 1u, NULL, NULL);
}

static void drain_outgoing(void)
{
    uint8_t frame[COMMS_FRAME_MAX];
    for (unsigned i = 0u; i < MQTT_FRAMES_PER_POLL && online; ++i)
    {
        unsigned n = comms_bridge_take(frame, sizeof frame);
        if (n == 0u)
        {
            return;
        }
        /* QoS 0, not retained: telemetry is replaced by the next one, never replayed. */
        if (mqtt_publish(client, MQTT_TOPIC, frame, (u16_t)n, 0u, 0u, NULL, NULL) != ERR_OK)
        {
            return;	/* send buffer full: the frame is dropped, the next one follows */
        }
    }
}

static void retry_timeout(void *arg)
{
    (void)arg;
    retry_pending = false;
}

static void attempt_timeout(void *arg)
{
    (void)arg;
    tm_printf((UB*)"mqtt: connection/subscription deadline; retry scheduled\n");
    drop_connection();
    schedule_retry();
}

static void incoming_publish(void *arg, const char *topic, u32_t length)
{
    (void)arg;
    (void)length;
    in_command_topic = strcmp(topic, MQTT_COMMAND_TOPIC) == 0;
    rx_length = 0u;
    rx_overflow = false;
}

static void incoming_data(void *arg, const u8_t *data, u16_t length, u8_t flags)
{
    (void)arg;
    if (!in_command_topic)
    {
        return;
    }
    if (rx_length + length > sizeof rx)
    {
        rx_overflow = true;	/* longer than any frame: refuse the whole message */
    }
    else
    {
        memcpy(&rx[rx_length], data, length);
        rx_length += length;
    }
    if ((flags & MQTT_DATA_FLAG_LAST) != 0u && !rx_overflow && rx_length > 0u)
    {
        comms_bridge_put(rx, rx_length);	/* the bridge consumer validates the content */
    }
}

static void subscribed(void *arg, err_t result)
{
    (void)arg;
    if (result != ERR_OK)
    {
        attempt_timeout(NULL);
        return;
    }
    sys_untimeout(attempt_timeout, NULL);
    connecting = false;
    retry_ms = MQTT_RETRY_MIN_MS;
    set_online(true);
    publish_status(STATUS_ONLINE);
    tm_printf((UB*)"mqtt: ready\n");
}

static void connection_cb(mqtt_client_t *c, void *arg, mqtt_connection_status_t status)
{
    (void)arg;
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        mqtt_set_inpub_callback(c, incoming_publish, incoming_data, NULL);
        if (mqtt_subscribe(c, MQTT_COMMAND_TOPIC, 0u, subscribed, NULL) != ERR_OK)
        {
            attempt_timeout(NULL);
        }
        return;
    }
    drop_connection();	/* refused or disconnected */
    schedule_retry();
}

static bool valid_topic(const char *topic)
{
    return topic != NULL && topic[0] != '\0' && strlen(topic) < MQTT_TOPIC_MAX;
}

static bool configure(void)
{
    uint8_t mac[6] = {0};
    if (!valid_topic(MQTT_TOPIC) || !valid_topic(MQTT_STATUS_TOPIC) || !valid_topic(MQTT_COMMAND_TOPIC) ||
        !ipaddr_aton(MQTT_BROKER_IP, &broker_addr))
    {
        return false;
    }
    cyw43_hal_get_mac(CYW43_HAL_MAC_WLAN0, mac);
    /* "<id>-<12 hex MAC digits>"; formatted by hand because newlib stdio
     * needs syscalls this kernel does not provide. */
    static const char hex[] = "0123456789abcdef";
    size_t len = strlen(MQTT_CLIENT_ID);
    if (len == 0 || len + 1 + 2 * sizeof mac >= sizeof client_id)
    {
        return false;
    }
    memcpy(client_id, MQTT_CLIENT_ID, len);
    client_id[len++] = '-';
    for (size_t i = 0; i < sizeof mac; i++)
    {
        client_id[len++] = hex[mac[i] >> 4];
        client_id[len++] = hex[mac[i] & 0x0f];
    }
    client_id[len] = '\0';
    return true;
}

static void connect_now(void)
{
    struct mqtt_connect_client_info_t info = {0};
    info.client_id = client_id;
    info.client_user = MQTT_USERNAME[0] != '\0' ? MQTT_USERNAME : NULL;
    info.client_pass = MQTT_PASSWORD[0] != '\0' ? MQTT_PASSWORD : NULL;
    info.keep_alive = MQTT_KEEPALIVE_S;
    info.will_topic = MQTT_STATUS_TOPIC;
    info.will_msg = STATUS_OFFLINE;
    info.will_qos = 1u;
    info.will_retain = 1u;
    connecting = true;
    sys_timeout(MQTT_ATTEMPT_MS, attempt_timeout, NULL);
    if (mqtt_client_connect(client, &broker_addr, MQTT_BROKER_PORT, connection_cb, NULL, &info) != ERR_OK)
    {
        attempt_timeout(NULL);
    }
}

void mqtt_utk_start(void)
{
    if (!configure())
    {
        bad_configuration = true;
        tm_printf((UB*)"mqtt: invalid broker/client/topic configuration\n");
        return;
    }
    client = mqtt_client_new();
    if (client == NULL)
    {
        bad_configuration = true;
        tm_printf((UB*)"mqtt: client allocation failed\n");
    }
}

void mqtt_utk_poll(bool link_up)
{
    if (bad_configuration || client == NULL)
    {
        return;
    }
    if (link_up != link_ready)
    {
        link_ready = link_up;
        if (!link_up)
        {
            cancel_timers();
            drop_connection();
            retry_ms = MQTT_RETRY_MIN_MS;
        }
        comms_bridge_link(link_ready, online);
    }
    if (!link_ready)
    {
        return;
    }
    if (!online && !connecting && !retry_pending)
    {
        connect_now();
    }
    else if (online)
    {
        if (!mqtt_client_is_connected(client))
        {
            drop_connection();
            schedule_retry();
        }
        else
        {
            drain_outgoing();
        }
    }
}
