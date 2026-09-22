#ifndef TEST_FAKE_SDK_H
#define TEST_FAKE_SDK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef int err_t;
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_TIMEOUT -3
#define ERR_ABRT -13
#define MQTT_DATA_FLAG_LAST 1
#define CYW43_ITF_STA 0
#define CYW43_LINK_UP 3
#define CYW43_AUTH_WPA2_AES_PSK 1
#define CYW43_AUTH_OPEN 0
#define PICO_UNIQUE_BOARD_ID_SIZE_BYTES 8
typedef struct { uint32_t addr; } ip_addr_t;
typedef struct { bool connected; } mqtt_client_t;
typedef enum { MQTT_CONNECT_ACCEPTED = 0, MQTT_CONNECT_DISCONNECTED = 256 } mqtt_connection_status_t;
typedef void (*mqtt_connection_cb_t)(mqtt_client_t *, void *, mqtt_connection_status_t);
typedef void (*mqtt_request_cb_t)(void *, err_t);
typedef void (*mqtt_incoming_publish_cb_t)(void *, const char *, u32_t);
typedef void (*mqtt_incoming_data_cb_t)(void *, const u8_t *, u16_t, u8_t);
struct mqtt_connect_client_info_t {
    const char *client_id, *client_user, *client_pass;
    u16_t keep_alive;
    const char *will_topic, *will_msg;
    u8_t will_msg_len, will_qos, will_retain;
};
struct netif { ip_addr_t address; };
extern struct netif *netif_default;
#define netif_ip4_addr(n) (&(n)->address)
extern int cyw43_state;
uint32_t get_absolute_time(void);
uint32_t to_ms_since_boot(uint32_t time);
int cyw43_arch_init(void);
void cyw43_arch_deinit(void);
void cyw43_arch_poll(void);
void cyw43_arch_enable_sta_mode(void);
void cyw43_arch_disable_sta_mode(void);
int cyw43_arch_wifi_connect_async(const char *, const char *, uint32_t);
int cyw43_tcpip_link_status(int *, int);
void pico_get_unique_board_id_string(char *, size_t);
uint64_t get_rand_64(void);
int ipaddr_aton(const char *, ip_addr_t *);
const char *ip4addr_ntoa(const ip_addr_t *);
mqtt_client_t *mqtt_client_new(void);
void mqtt_disconnect(mqtt_client_t *);
err_t mqtt_client_connect(mqtt_client_t *, const ip_addr_t *, u16_t,
                          mqtt_connection_cb_t, void *, const struct mqtt_connect_client_info_t *);
void mqtt_set_inpub_callback(mqtt_client_t *, mqtt_incoming_publish_cb_t,
                            mqtt_incoming_data_cb_t, void *);
err_t mqtt_subscribe(mqtt_client_t *, const char *, u8_t, mqtt_request_cb_t, void *);
err_t mqtt_publish(mqtt_client_t *, const char *, const void *, u16_t, u8_t, u8_t,
                   mqtt_request_cb_t, void *);
#endif
