#ifndef FAKE_MQTT_H
#define FAKE_MQTT_H
#include <stdint.h>
#include <stddef.h>
#include "lwip/ip_addr.h"
typedef int err_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
#define ERR_OK 0
#define ERR_CONN (-11)
#define ERR_MEM (-1)
typedef enum { MQTT_CONNECT_ACCEPTED=0, MQTT_CONNECT_REFUSED_SERVER=3, MQTT_CONNECT_DISCONNECTED=256 } mqtt_connection_status_t;
typedef struct mqtt_client_s mqtt_client_t;
typedef void (*mqtt_connection_cb_t)(mqtt_client_t *,void *,mqtt_connection_status_t);
typedef void (*mqtt_request_cb_t)(void *,err_t);
struct mqtt_connect_client_info_t { const char *client_id; uint16_t keep_alive; };
mqtt_client_t *mqtt_client_new(void);
int mqtt_client_is_connected(mqtt_client_t *client);
err_t mqtt_client_connect(mqtt_client_t *client,const ip_addr_t *address,u16_t port,
                         mqtt_connection_cb_t callback,void *arg,const struct mqtt_connect_client_info_t *info);
err_t mqtt_publish(mqtt_client_t *client,const char *topic,const void *payload,u16_t length,
                   unsigned qos,unsigned retain,mqtt_request_cb_t callback,void *arg);
#endif
