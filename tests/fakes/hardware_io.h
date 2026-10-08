#ifndef HARDWARE_IO_H
#define HARDWARE_IO_H
#include <stdint.h>
#define RESETS_RESET 0x4000c000u
#define RESETS_RESET_DONE 0x4000c008u
#define RESETS_RESET_ADC 1u
#define TIMER_TIMERAWL 0x40054028u
#define GPIO_CTRL(p) (0x40014004u+(p)*8u)
#define GPIO(p) (0x4001c004u+(p)*4u)
#define GPIO_IE (1u<<6)
#define GPIO_OD (1u<<7)
#define GPIO_PUE (1u<<3)
#define GPIO_PDE (1u<<2)
uint32_t in_w(uint32_t address);
void out_w(uint32_t address, uint32_t value);
void clr_w(uint32_t address, uint32_t bits);
void set_w(uint32_t address, uint32_t bits);
#endif
