/* Read-only robot telemetry. All calls run in the existing core-0 radio task.
 * Application tasks only update the S1 mutex-protected snapshot. */
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#include "wifi_secrets.h"
#include "mqtt_utk.h"
#include "../../../../app_program/subsystem1/telemetry.h"
#include <tm/tmonitor.h>

#define MQTT_UTK_KEEPALIVE_S 60
#define MQTT_UTK_RETRY_MS 3000
static mqtt_client_t *client;
static ip_addr_t broker_addr;
static bool connecting, retry_pending, bad_configuration;
static void publish_timeout(void *arg);
static void retry_timeout(void *arg);

static void schedule_retry(void) {
    sys_untimeout(publish_timeout, NULL);
    sys_untimeout(retry_timeout, NULL);
    connecting = false;
    retry_pending = true;
    sys_timeout(MQTT_UTK_RETRY_MS, retry_timeout, NULL);
}
static void publish_done(void *arg, err_t result) {
    (void)arg;
    if (result != ERR_OK) tm_printf((UB*)"mqtt: publication failed, err=%d\n", (int)result);
}
static void publish_latest(void) {
    S1Telemetry snapshot;
    char payload[S1_TELEMETRY_PAYLOAD_MAX + 1];
    if (!s1_telemetry_copy(&snapshot)) return;
    unsigned length = s1_telemetry_encode(&snapshot, sys_now(), payload, sizeof(payload));
    if (!length) return;
    err_t result = mqtt_publish(client, MQTT_TOPIC, payload, (u16_t)length,
                               0 /* QoS */, 0 /* retain */, publish_done, NULL);
    if (result != ERR_OK) tm_printf((UB*)"mqtt: publication skipped, err=%d\n", (int)result);
}
static void publish_timeout(void *arg) {
    (void)arg;
    if (!client || !mqtt_client_is_connected(client)) return;
    publish_latest();
    sys_timeout(S1_TELEMETRY_PERIOD_MS, publish_timeout, NULL);
}
static void retry_timeout(void *arg) {
    (void)arg;
    retry_pending = false;
    mqtt_utk_start();
}
static void connection_cb(mqtt_client_t *c, void *arg, mqtt_connection_status_t status) {
    (void)c; (void)arg;
    sys_untimeout(publish_timeout, NULL);
    sys_untimeout(retry_timeout, NULL);
    connecting = retry_pending = false;
    if (status == MQTT_CONNECT_ACCEPTED) {
        tm_printf((UB*)"mqtt: connected; read-only telemetry on " MQTT_TOPIC "\n");
        publish_latest(); /* No queued history or echo subscription. */
        sys_timeout(S1_TELEMETRY_PERIOD_MS, publish_timeout, NULL);
    } else {
        tm_printf((UB*)"mqtt: connection status %d; retry in 3s\n", (int)status);
        schedule_retry();
    }
}
void mqtt_utk_start(void) {
    if (bad_configuration || connecting || retry_pending ||
        (client && mqtt_client_is_connected(client))) return;
    if (!ip4addr_aton(MQTT_BROKER_IP, ip_2_ip4(&broker_addr))) {
        bad_configuration = true;
        tm_printf((UB*)"mqtt: invalid broker IPv4 configuration\n");
        return;
    }
    IP_SET_TYPE(&broker_addr, IPADDR_TYPE_V4);
    if (!client) {
        client = mqtt_client_new();
        if (!client) { schedule_retry(); return; }
    }
    struct mqtt_connect_client_info_t info = {0};
    info.client_id = MQTT_CLIENT_ID;
    info.keep_alive = MQTT_UTK_KEEPALIVE_S;
    connecting = true;
    err_t result = mqtt_client_connect(client, &broker_addr, MQTT_BROKER_PORT,
                                      connection_cb, NULL, &info);
    if (result != ERR_OK) {
        tm_printf((UB*)"mqtt: connect request failed, err=%d; retry in 3s\n", (int)result);
        schedule_retry();
    }
}
