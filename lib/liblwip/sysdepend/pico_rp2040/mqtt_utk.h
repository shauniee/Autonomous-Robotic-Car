#ifndef MQTT_UTK_H
#define MQTT_UTK_H
/* Call only from the kernel radio service while the link is up. Poll calls
   cannot bypass a connection attempt or the 3-second retry backoff. */
void mqtt_utk_start(void);
#endif
