#ifndef FAKE_IP_H
#define FAKE_IP_H
#include <stdint.h>
typedef struct { uint32_t value; } ip_addr_t;
#define ip_2_ip4(p) (p)
#define IPADDR_TYPE_V4 0
#define IP_SET_TYPE(p,t) ((void)(p),(void)(t))
int ip4addr_aton(const char *text,ip_addr_t *address);
#endif
