#ifndef FAKE_TIMEOUTS_H
#define FAKE_TIMEOUTS_H
#include <stdint.h>
typedef void (*sys_timeout_handler)(void *);
void sys_timeout(uint32_t delay,sys_timeout_handler handler,void *arg);
void sys_untimeout(sys_timeout_handler handler,void *arg);
#endif
