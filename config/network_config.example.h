/* HOW TO READ THIS FILE
   #define NAME value gives a setting a name before the C program is compiled.
   Text values need double quotes; numbers such as 1883 and 1 do not.
   Changes take effect only after rebuilding and flashing the firmware.
   Edit network_config.h for your real settings; the .example.h is the shareable
   template. network_config.h is ignored by Git because it can contain passwords.
*/
#ifndef ROBOT_NETWORK_CONFIG_H
/* Header guard: prevents these settings being included more than once per C source file. */
#define ROBOT_NETWORK_CONFIG_H

/* Copy to config/network_config.h (gitignored), then edit locally.
   Empty SSID leaves networking disabled; empty broker enables WiFi-only bring-up. */
/* The WiFi network name, exactly as shown on your phone/laptop.
   SSID means Service Set Identifier. An empty string disables networking. */
#define ROBOT_WIFI_SSID ""
/* Password for that WiFi network. This is different from the MQTT password below.
   An empty string selects open WiFi authentication in the current code. */
#define ROBOT_WIFI_PASSWORD ""
/* Numeric IPv4 LAN address of your MQTT broker, e.g. "192.168.1.20".
   This initial transport intentionally does not use DNS or TLS. */
/* LAN IPv4 address of the computer running the MQTT message server (broker).
   Example: "192.168.1.20". Do not use the Pico IP or "localhost" here.
   An empty string skips MQTT but still joins WiFi when SSID is set.
   Hostnames are not supported by this backend. */
#define ROBOT_MQTT_BROKER_IP ""
/* TCP port where the broker listens. 1883 is the usual plain-MQTT port.
   It must match the listener port in mosquitto-lab.conf. Changing this number
   to 8883 alone does not enable encrypted TLS connections. */
#define ROBOT_MQTT_PORT 1883
/* Optional login name for the MQTT broker, not your WiFi network.
   Leave empty for the supplied anonymous lab broker. Empty means no broker login. */
#define ROBOT_MQTT_USERNAME ""
/* Password for the MQTT username above. It is used only when that username
   is non-empty. Leave empty for the supplied anonymous lab broker. */
#define ROBOT_MQTT_PASSWORD ""
/* Shared prefix for this car's MQTT message channels. For example, the code
   adds "/telemetry", "/heartbeat", "/command" and "/status" to this string.
   Use a different root for each car, and the same root in the laptop tool. */
#define ROBOT_MQTT_TOPIC_ROOT "robot/car01"
/* Local autonomy continues on network loss; subscription accepts status only. */

#endif
