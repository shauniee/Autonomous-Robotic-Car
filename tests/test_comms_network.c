/* Fake SDK drives the real backend through WiFi/MQTT failures and callbacks.
   These tests do not exercise RF, TCP or a real broker. */
#include "fake_sdk.h"
#include "robot/subsystems.h"
#include "robot/controller.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

int cyw43_state;
static struct netif interface;
struct netif *netif_default = &interface;
static mqtt_client_t client;
static uint32_t now;
static int link, joins, connects, subscriptions, publishes;
static bool fail_publish;
static mqtt_connection_cb_t connection_cb;
static mqtt_request_cb_t subscribe_cb;
static mqtt_incoming_publish_cb_t incoming_pub;
static mqtt_incoming_data_cb_t incoming_data_cb;
static char heartbeat[512], telemetry[2048];

uint32_t get_absolute_time(void) { return now; }
uint32_t to_ms_since_boot(uint32_t time) { return time; }
int cyw43_arch_init(void) { return 0; }
void cyw43_arch_deinit(void) { }
void cyw43_arch_poll(void) { }
void cyw43_arch_enable_sta_mode(void) { }
void cyw43_arch_disable_sta_mode(void) { link = 0; }
int cyw43_arch_wifi_connect_async(const char *ssid, const char *password, uint32_t auth) {
    assert(ssid[0] && password[0] && auth == CYW43_AUTH_WPA2_AES_PSK);
    ++joins;
    return 0;
}
int cyw43_tcpip_link_status(int *state, int itf) { (void)state; (void)itf; return link; }
void pico_get_unique_board_id_string(char *out, size_t capacity) { snprintf(out, capacity, "1234567890abcdef"); }
uint64_t get_rand_64(void) { static uint64_t value = 10; return ++value; }
int ipaddr_aton(const char *text, ip_addr_t *ip) { (void)text; ip->addr = 1; return 1; }
const char *ip4addr_ntoa(const ip_addr_t *ip) { (void)ip; return "192.0.2.2"; }
mqtt_client_t *mqtt_client_new(void) { return &client; }
void mqtt_disconnect(mqtt_client_t *c) { c->connected = false; }
err_t mqtt_client_connect(mqtt_client_t *c, const ip_addr_t *ip, u16_t port,
                          mqtt_connection_cb_t cb, void *arg, const struct mqtt_connect_client_info_t *info) {
    (void)c; (void)ip; (void)arg;
    assert(port == 1883 && info->will_retain && info->keep_alive > 0);
    assert(strcmp(info->will_msg, "{\"online\":false}") == 0);
    connection_cb = cb;
    incoming_pub = NULL; /* Real lwIP resets callbacks at connect. */
    incoming_data_cb = NULL;
    ++connects;
    return ERR_OK;
}
void mqtt_set_inpub_callback(mqtt_client_t *c, mqtt_incoming_publish_cb_t pub,
                            mqtt_incoming_data_cb_t data, void *arg) {
    (void)c; (void)arg;
    incoming_pub = pub;
    incoming_data_cb = data;
}
err_t mqtt_subscribe(mqtt_client_t *c, const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg) {
    (void)c; (void)arg;
    assert(strcmp(topic, "robot/car01/command") == 0 && qos == 1);
    subscribe_cb = cb;
    ++subscriptions;
    return ERR_OK;
}
err_t mqtt_publish(mqtt_client_t *c, const char *topic, const void *payload, u16_t length,
                   u8_t qos, u8_t retain, mqtt_request_cb_t cb, void *arg) {
    (void)c; (void)qos; (void)cb; (void)arg;
    if (fail_publish) return ERR_MEM;
    ++publishes;
    if (strstr(topic, "/heartbeat")) {
        assert(!retain && length < sizeof(heartbeat));
        memcpy(heartbeat, payload, length); heartbeat[length] = 0;
    } else if (strstr(topic, "/telemetry")) {
        assert(!retain && length < sizeof(telemetry));
        memcpy(telemetry, payload, length); telemetry[length] = 0;
    }
    return ERR_OK;
}

static RobotCommand tick(uint32_t time) { now = time; return comms_poll(now); }
static void accept_connection(void) {
    assert(connection_cb && incoming_pub && incoming_data_cb);
    client.connected = true;
    connection_cb(&client, NULL, MQTT_CONNECT_ACCEPTED);
    assert(subscribe_cb);
    subscribe_cb(NULL, ERR_OK);
}
static void send(const char *payload) {
    incoming_pub(NULL, "robot/car01/command", (u32_t)strlen(payload));
    incoming_data_cb(NULL, (const u8_t *)payload, 2, 0);
    incoming_data_cb(NULL, (const u8_t *)payload + 2, (u16_t)(strlen(payload) - 2), MQTT_DATA_FLAG_LAST);
}
static void make_start(char *out, size_t capacity, uint32_t id) {
    char session[17];
    const char *value = strstr(heartbeat, "\"session\":\"");
    assert(value);
    memcpy(session, value + strlen("\"session\":\""), 16); session[16] = 0;
    snprintf(out, capacity, "{\"session\":\"%s\",\"id\":%" PRIu32
             ",\"issued_ms\":%" PRIu32 ",\"command\":\"start\"}", session, id, now);
}

int main(void) {
    comms_init();
    assert(tick(0) == COMMAND_NONE && joins == 1);
    assert(tick(19999) == COMMAND_NONE && joins == 1);
    tick(20000); /* Join timeout, retry after backoff. */
    tick(20999); assert(joins == 1);
    tick(21000); assert(joins == 2);
    link = CYW43_LINK_UP;
    tick(21020); tick(21040); assert(connects == 1);
    tick(36040); /* Broker connect timeout. */
    tick(38040); assert(connects == 2);
    connection_cb(&client, NULL, MQTT_CONNECT_ACCEPTED);
    subscribe_cb(NULL, ERR_ABRT); /* Subscription denied, retry whole connection. */
    tick(38060); tick(42060); assert(connects == 3);
    accept_connection();
    assert(tick(42080) == COMMAND_NONE);
    assert(strstr(heartbeat, "\"session\":") && subscriptions == 2);
    char start[192];
    make_start(start, sizeof(start), 1);
    send(start);
    assert(tick(42100) == COMMAND_START);
    send(start); assert(tick(42120) == COMMAND_NONE); /* Duplicate. */
    make_start(start, sizeof(start), 2);
    send(start);
    send("stop");
    assert(tick(42140) == COMMAND_STOP);

    VehicleController c; controller_init(&c);
    SensorFrame s = {0}; MotionFeedback m = {0};
    comms_publish(&c, &s, &m, now);
    assert(strstr(telemetry, "\"front_distance_m\":null"));
    fail_publish = true;
    comms_publish(&c, &s, &m, now); /* Full buffers must not wait or retry in a loop. */
    fail_publish = false;
    tick(43140);
    assert(strstr(heartbeat, "\"dropped_publish\":1"));

    make_start(start, sizeof(start), 3);
    send(start); /* Pending start must be cleared on link loss. */
    link = 0;
    assert(tick(43160) == COMMAND_STOP);
    assert(tick(43180) == COMMAND_NONE);
    int before = publishes;
    comms_publish(&c, &s, &m, now);
    assert(publishes == before); /* Offline telemetry is not queued. */
    tick(44160); link = CYW43_LINK_UP;
    tick(44180); tick(44200);
    accept_connection(); tick(44220);
    send(start); /* Old session is rejected even when timestamp is still fresh. */
    assert(tick(44240) == COMMAND_NONE);
    make_start(start, sizeof(start), 1); send(start);
    assert(tick(44260) == COMMAND_START);
    connection_cb(&client, NULL, MQTT_CONNECT_DISCONNECTED);
    assert(tick(44280) == COMMAND_STOP);
    tick(45280); accept_connection(); tick(45300);
    assert(subscriptions == 4); /* Subscription restored after broker disconnect. */
    puts("Pico W backend failure/reconnect checks passed (fake SDK).");
    return 0;
}
