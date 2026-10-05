/* Buddy 1's Pico W/WH implementation (Pico WH on Robo Pico).
   main_pico.c calls init once, poll every loop, and publish periodically.
   WiFi joins the local network; MQTT connects to the broker on that network.
   A callback is a function the networking library calls when an event happens,
   such as receiving a message or completing a connection attempt. */
#include "robot/subsystems.h"     /* Shared function declarations and sensor/controller types. */
#include "robot/comms_protocol.h" /* JSON formatting and the validated command inbox. */
#include "network_config.h"       /* Your local WiFi name, passwords and broker settings. */
#include "pico/stdlib.h"          /* Pico time functions and basic SDK facilities. */
#include "pico/cyw43_arch.h"      /* Driver for the Pico W's onboard wireless chip. */
#include "pico/unique_id.h"       /* Board identifier, used to name this MQTT client. */
#include "pico/rand.h"            /* Random values for new command sessions. */
#include "lwip/apps/mqtt.h"      /* lwIP is the networking stack; this is its MQTT API. */
#include "lwip/netif.h"          /* Network interface information, including our IP. */
#include <inttypes.h>            /* Portable printf formats for fixed-width integers. */
#include <stdio.h>               /* printf, puts and snprintf for output/formatting. */
#include <string.h>              /* strlen, strcmp and strchr for handling text. */

/* Polling architecture: callbacks and application calls run on the same core.
   Do not call this backend from interrupts or a second core. */
typedef enum { NET_DISABLED, WIFI_WAIT, WIFI_JOIN, WIFI_READY, MQTT_WAIT,
               MQTT_CONNECTING, MQTT_SUBSCRIBING, NET_ONLINE } NetworkState;
/* These named states describe progress:
   DISABLED -> no network work; WIFI_WAIT -> wait until retry time;
   WIFI_JOIN -> association/IP setup; WIFI_READY -> WiFi only, no broker configured;
   MQTT_WAIT -> ready to try the broker;
   MQTT_CONNECTING -> wait for connection result;
   MQTT_SUBSCRIBING -> wait for permission to receive commands;
   NET_ONLINE -> publish data and accept commands. */

#define WIFI_TIMEOUT_MS 20000u  /* Allow 20 seconds per WiFi connection attempt. */
#define MQTT_TIMEOUT_MS 15000u  /* Allow 15 seconds for broker connection/subscription. */
#define HEARTBEAT_MS 1000u      /* Send an "I am alive" message every second. */
#define MAX_BACKOFF_MS 30000u   /* Retry delay doubles after failures, up to 30 seconds. */
#define TOPIC_MAX 96u          /* Maximum topic buffer size, including its final '\0'. */

/* 'static' keeps this data between calls and limits its visibility to this file.
   A struct groups related fields. 'net.field' accesses one of these fields.
   Static storage starts zero-initialized, including counters and flags. */
static struct {
    NetworkState state;                       /* Current stage in the connection process. */
    mqtt_client_t *client;                    /* Pointer to the MQTT client allocated by lwIP. */
    ip_addr_t broker;                         /* Broker IP converted from text to network form. */
    struct mqtt_connect_client_info_t info;   /* Login, keepalive and last-will settings. */
    char client_id[40];                       /* Unique name sent to the broker. */
    char telemetry_topic[TOPIC_MAX], heartbeat_topic[TOPIC_MAX];
    char command_topic[TOPIC_MAX], status_topic[TOPIC_MAX];
    CommandInbox inbox;                       /* Receives, validates and queues commands. */
    uint32_t deadline_ms, backoff_ms, heartbeat_ms; /* Deadline, retry delay, last heartbeat time. */
    uint32_t dropped_publish, rejected_commands;  /* Diagnostics reported in heartbeat JSON. */
    bool reconnect_requested, announce_online; /* Recovery/status flags. */
    char telemetry[COMMS_TELEMETRY_MAX];       /* Reused output buffer; no growing message queue. */
} net;

/* Convert the SDK's time representation to milliseconds since the board booted. */
static uint32_t clock_ms(void) { return to_ms_since_boot(get_absolute_time()); }
static bool due(uint32_t now, uint32_t deadline) {
    /* Signed difference handles a wrapping 32-bit clock for our short deadlines. */
    return (int32_t)(now - deadline) >= 0;
}

static bool topic(char *out, const char *suffix) {
    /* Example: "robot/car01" + "/" + "command" -> "robot/car01/command".
       snprintf limits writes; its return value reveals whether text was cut off. */
    int n = snprintf(out, TOPIC_MAX, "%s/%s", ROBOT_MQTT_TOPIC_ROOT, suffix);
    return n > 0 && (size_t)n < TOPIC_MAX;
}

static void publish_done(void *arg, err_t error) {
    /* The library requires an argument slot, but this callback does not use it.
       (void)arg marks that intentional; ERR_OK means the request succeeded. */
    (void)arg;
    if (error != ERR_OK) ++net.dropped_publish;
}

static bool publish(const char *name, const char *data, bool retained) {
    /* QoS 0 sends without an MQTT delivery acknowledgement; QoS 1 requests one
       and may deliver duplicates. Here status uses QoS 1 and retain=true.
       Retain asks the broker to remember the latest message for new subscribers.
       Telemetry/heartbeat use QoS 0 and retain=false because newer samples follow.
       publish_done is the callback; NULL means no extra callback argument. */
    if (mqtt_publish(net.client, name, data, (u16_t)strlen(data), retained ? 1 : 0,
                     retained, publish_done, NULL) == ERR_OK) return true;
    ++net.dropped_publish;
    return false; /* Drop this sample; never build an unbounded offline backlog. */
}

static void schedule_retry(uint32_t now, bool wifi_lost) {
    /* The Word design keeps local autonomy independent of network loss. */
    /* Stop trusting incoming commands before closing the old connection. */
    net.state = wifi_lost ? WIFI_WAIT : MQTT_WAIT;
    if (net.client) mqtt_disconnect(net.client);
    command_inbox_reset(&net.inbox, "");
    net.reconnect_requested = false;
    net.announce_online = false;
    net.deadline_ms = now + net.backoff_ms;
    /* Set a future retry time instead of sleeping here and blocking the car. */
    if (net.backoff_ms < MAX_BACKOFF_MS) net.backoff_ms *= 2u;
    if (net.backoff_ms > MAX_BACKOFF_MS) net.backoff_ms = MAX_BACKOFF_MS;
    if (wifi_lost) {
        /* Station mode means joining an existing WiFi network, not hosting one.
           Restart it only when WiFi itself needs recovery. */
        cyw43_arch_disable_sta_mode();
        cyw43_arch_enable_sta_mode();
    }
    printf("[net] %s retry scheduled\n", wifi_lost ? "WiFi" : "MQTT");
}

static void incoming_publish(void *arg, const char *name, u32_t length) {
    (void)arg;
    /* strcmp returns zero for equal strings. Only our command topic is accepted. */
    const bool correct = net.state == NET_ONLINE && strcmp(name, net.command_topic) == 0;
    command_inbox_begin(&net.inbox, length, correct);
    if (correct && !net.inbox.receiving) ++net.rejected_commands;
}

static void incoming_data(void *arg, const u8_t *data, u16_t length, u8_t flags) {
    (void)arg;
    if (!net.inbox.receiving) return;
    /* MQTT may deliver a payload in pieces. '&' tests the LAST flag bit so the
       inbox knows when it has enough data to parse one complete command. */
    if (!command_inbox_data(&net.inbox, data, length,
                            (flags & MQTT_DATA_FLAG_LAST) != 0, clock_ms())) {
        ++net.rejected_commands;
    }
}

static void subscribed(void *arg, err_t error) {
    (void)arg;
    if (net.state != MQTT_SUBSCRIBING) return;
    /* Connecting to MQTT is not enough: wait until command subscription succeeds. */
    if (error != ERR_OK) { net.reconnect_requested = true; return; }
    net.state = NET_ONLINE;
    net.backoff_ms = 1000u;
    net.heartbeat_ms = clock_ms() - HEARTBEAT_MS;
    /* Make the first heartbeat due immediately so the laptop learns the session. */
    net.announce_online = true;
    puts("[net] MQTT online; command subscription ready");
}

static void connected(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    (void)client;
    (void)arg;
    if (status != MQTT_CONNECT_ACCEPTED) {
        printf("[net] MQTT connection status %d\n", (int)status);
        net.reconnect_requested = true;
        return;
    }
    char session[COMMS_SESSION_LENGTH + 1];
    /* Format a random 64-bit number as 16 hex characters. A new session prevents
       a status request recorded during a previous connection from being accepted now.
       PRIx64 supplies the right printf format for this integer type. */
    snprintf(session, sizeof(session), "%016" PRIx64, get_rand_64());
    command_inbox_reset(&net.inbox, session);
    net.state = MQTT_SUBSCRIBING;
    net.deadline_ms = clock_ms() + MQTT_TIMEOUT_MS;
    /* Subscribe at QoS 1. The inbox's increasing IDs handle duplicate commands. */
    if (mqtt_subscribe(net.client, net.command_topic, 1, subscribed, NULL) != ERR_OK) {
        net.reconnect_requested = true;
    }
}

void comms_init(void) {
    net.state = NET_DISABLED;
    net.backoff_ms = 1000u;
    /* Index [0] is a string's first character; zero here means the string is empty. */
    if (!ROBOT_WIFI_SSID[0]) {
        puts("[net] Disabled: fill config/network_config.h and rebuild");
        return;
    }
    /* Convert the broker address, check port range, reject wildcard topic roots,
       and build all four topic names before starting any network activity. */
    const bool mqtt_enabled = ROBOT_MQTT_BROKER_IP[0] != '\0';
    if (mqtt_enabled && (!ipaddr_aton(ROBOT_MQTT_BROKER_IP, &net.broker) ||
        ROBOT_MQTT_PORT < 1 || ROBOT_MQTT_PORT > 65535 ||
        !ROBOT_MQTT_TOPIC_ROOT[0] || strchr(ROBOT_MQTT_TOPIC_ROOT, '#') ||
        strchr(ROBOT_MQTT_TOPIC_ROOT, '+') ||
        !topic(net.telemetry_topic, "telemetry") ||
        !topic(net.heartbeat_topic, "heartbeat") ||
        !topic(net.command_topic, "command") || !topic(net.status_topic, "status"))) {
        puts("[net] Disabled: invalid broker IP, port or topic root");
        return;
    }
    /* Set up the wireless chip and network stack; zero is SDK success here. */
    if (cyw43_arch_init() != 0) {
        puts("[net] CYW43 initialization failed");
        return;
    }
    net.client = mqtt_enabled ? mqtt_client_new() : NULL;
    if (mqtt_enabled && !net.client) {
        puts("[net] MQTT allocation failed");
        cyw43_arch_deinit();
        return;
    }
    /* Each byte needs two hexadecimal characters, plus one string terminator.
       A board-based client ID avoids different cars replacing each other's login. */
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));
    snprintf(net.client_id, sizeof(net.client_id), "robot-%s", board_id);
    net.info = (struct mqtt_connect_client_info_t){
        /* .field = value initializes a named field in this settings structure.
           condition ? a : b chooses a when true and b when false. */
        .client_id = net.client_id,
        .client_user = ROBOT_MQTT_USERNAME[0] ? ROBOT_MQTT_USERNAME : NULL,
        .client_pass = ROBOT_MQTT_USERNAME[0] ? ROBOT_MQTT_PASSWORD : NULL,
        .keep_alive = 10, /* MQTT keepalive interval in seconds, separate from heartbeat. */
        .will_topic = net.status_topic,
        .will_msg = "{\"online\":false}", /* Broker sends this after unexpected connection loss. */
        .will_qos = 1,
        .will_retain = 1
    };
    cyw43_arch_enable_sta_mode();
    net.state = WIFI_WAIT;
    net.deadline_ms = clock_ms();
    puts("[net] Pico W/WH networking initialized");
    if (!mqtt_enabled) puts("[net] WiFi only: configure broker IP and rebuild for MQTT");
}

RobotCommand comms_poll(uint32_t now_ms) {
    if (net.state == NET_DISABLED) return COMMAND_NONE;
    cyw43_arch_poll(); /* Process pending WiFi/lwIP work; this can invoke our callbacks. */
    now_ms = clock_ms();
    int link = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA); /* WiFi/IP readiness. */

    if (net.state == WIFI_WAIT) {
        if (due(now_ms, net.deadline_ms)) {
            puts("[net] Joining WiFi");
            /* async starts the attempt and returns without waiting for connection.
               Later polls check link status or the attempt's timeout. */
            int result = cyw43_arch_wifi_connect_async(ROBOT_WIFI_SSID,
                ROBOT_WIFI_PASSWORD, ROBOT_WIFI_PASSWORD[0] ? CYW43_AUTH_WPA2_AES_PSK : CYW43_AUTH_OPEN);
            net.state = WIFI_JOIN;
            net.deadline_ms = now_ms + WIFI_TIMEOUT_MS;
            if (result != 0) schedule_retry(now_ms, true);
        }
    } else if (net.state == WIFI_JOIN) {
        if (link == CYW43_LINK_UP) {
            printf("[net] WiFi connected; IP %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
            net.state = net.client ? MQTT_WAIT : WIFI_READY;
            if (!net.client) net.backoff_ms = 1000u;
            net.deadline_ms = now_ms;
        } else if (link < 0 || due(now_ms, net.deadline_ms)) {
            schedule_retry(now_ms, true);
        }
    } else if (link != CYW43_LINK_UP) {
        /* Broker work depends on WiFi/IP, so recover that layer first. */
        schedule_retry(now_ms, true);
    } else if (net.reconnect_requested) {
        schedule_retry(now_ms, false);
    } else if (net.state == MQTT_WAIT && due(now_ms, net.deadline_ms)) {
        net.state = MQTT_CONNECTING;
        net.deadline_ms = now_ms + MQTT_TIMEOUT_MS;
        puts("[net] Connecting to MQTT broker");
        /* Start the broker connection; connected() receives its eventual result. */
        err_t result = mqtt_client_connect(net.client, &net.broker, ROBOT_MQTT_PORT,
                                           connected, NULL, &net.info);
        /* lwIP resets the client structure in connect(), so register after it. */
        mqtt_set_inpub_callback(net.client, incoming_publish, incoming_data, NULL);
        if (result != ERR_OK) schedule_retry(now_ms, false);
    } else if ((net.state == MQTT_CONNECTING || net.state == MQTT_SUBSCRIBING) &&
               due(now_ms, net.deadline_ms)) {
        schedule_retry(now_ms, false);
    }

    if (net.state == NET_ONLINE && (uint32_t)(now_ms - net.heartbeat_ms) >= HEARTBEAT_MS) {
        /* An application heartbeat lets the laptop see freshness, session and
           counters. It is separate from MQTT's internal keepalive traffic. */
        net.heartbeat_ms = now_ms;
        if (net.announce_online) {
            net.announce_online = !publish(net.status_topic, "{\"online\":true}", true);
        }
        char payload[320];
        /* The backslashes in \" below escape quotes in a C string literal;
           the transmitted JSON contains ordinary double quotes. PRIu32 inserts
           the platform's printf format for an unsigned 32-bit number. */
        snprintf(payload, sizeof(payload),
                 "{\"schema\":1,\"online\":true,\"session\":\"%s\",\"uptime_ms\":%" PRIu32
                 ",\"last_command_id\":%" PRIu32 ",\"rejected_commands\":%" PRIu32
                 ",\"dropped_publish\":%" PRIu32 "}", net.inbox.session, now_ms,
                 net.inbox.last_id, net.rejected_commands, net.dropped_publish);
        publish(net.heartbeat_topic, payload, false);
    }
    /* Consume once. The caller gives this to the controller; no motor writes here. */
    return COMMAND_NONE; /* Status-only MQTT, per the Word design. */
}

void comms_publish(const VehicleController *c, const SensorFrame *s,
                   const MotionFeedback *motion, uint32_t now_ms) {
    /* c = controller decisions, s = sensor snapshot, motion = encoder feedback.
       The encoder builds bounded JSON; zero length means it could not fit. */
    size_t length = comms_encode_telemetry(net.telemetry, sizeof(net.telemetry), c, s, motion, now_ms);
    if (!length) { ++net.dropped_publish; return; }
    /* USB remains useful before the broker/network is available. */
    puts(net.telemetry);
    if (net.state == NET_ONLINE) publish(net.telemetry_topic, net.telemetry, false);
}

void comms_publish_json(const char *json) {
    if (!json || !json[0] || strlen(json) >= COMMS_TELEMETRY_MAX) { ++net.dropped_publish; return; }
    puts(json);
    if (net.state == NET_ONLINE) publish(net.telemetry_topic, json, false);
}

bool comms_status_requested(void) {
    bool requested = net.inbox.status_requested;
    net.inbox.status_requested = false;
    return requested;
}
