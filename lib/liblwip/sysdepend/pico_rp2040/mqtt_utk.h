#ifndef MQTT_UTK_H
#define MQTT_UTK_H
#include <stdbool.h>
/* Kernel radio service only. Poll cannot bypass attempts or retry backoff. */
void mqtt_utk_start(void);
void mqtt_utk_poll(bool link_up);
#endif
