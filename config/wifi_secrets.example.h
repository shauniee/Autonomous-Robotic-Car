/*
 *	wifi_secrets.example.h
 *
 *	Copy this file to wifi_secrets.h (same directory) and fill in your own
 *	network's credentials. wifi_secrets.h is listed in .gitignore -- it
 *	will never be committed, so it's safe to put your real password there.
 *
 *	Only used when built with NET=lwip (see build_make/pico_rp2040.mk and
 *	lib/libwifi/sysdepend/pico_rp2040/cyw43_utk.c).
 */

#ifndef WIFI_SECRETS_H
#define WIFI_SECRETS_H

#define WIFI_SSID     "your-network-name"
#define WIFI_PASSWORD "your-network-password"

/* Only used when built with MQTT=1 (also requires NET=lwip). */
#define MQTT_BROKER_IP   "192.168.1.10"
#define MQTT_BROKER_PORT 1883
/* The service appends the radio MAC to this client-ID prefix. */
#define MQTT_CLIENT_ID   "csc01-s3-pico"
#define MQTT_USERNAME    ""
#define MQTT_PASSWORD    ""
#define MQTT_TOPIC           "robot/s3/telemetry"
#define MQTT_HEARTBEAT_TOPIC "robot/s3/heartbeat"
#define MQTT_STATUS_TOPIC    "robot/s3/status"
#define MQTT_REQUEST_TOPIC   "robot/s3/request"

#endif /* WIFI_SECRETS_H */

/* Configure all topics together when changing the robot/s3 root. */
#ifndef MQTT_MAPPING_TOPIC
#define MQTT_MAPPING_TOPIC "robot/s3/mapping"
#endif
#ifndef MQTT_MAPPING_STATUS_TOPIC
#define MQTT_MAPPING_STATUS_TOPIC "robot/s3/mapping_status"
#endif
