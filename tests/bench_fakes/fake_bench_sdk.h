#ifndef FAKE_BENCH_SDK_H
#define FAKE_BENCH_SDK_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define GPIO_OUT 1
#define GPIO_IN 0
#define GPIO_FUNC_I2C 3
#define GPIO_FUNC_PWM 4
#define GPIO_IRQ_EDGE_RISE 8u
#define GPIO_IRQ_EDGE_FALL 4u
#define clk_sys 0
#define i2c0 ((void*)0)
typedef struct { float divider; unsigned wrap; } pwm_config;
uint32_t get_absolute_time(void);
uint32_t to_ms_since_boot(uint32_t t);
uint32_t time_us_32(void);
void busy_wait_us_32(uint32_t us);
void gpio_init(unsigned pin);
void gpio_put(unsigned pin, bool level);
bool gpio_get(unsigned pin);
void gpio_set_dir(unsigned pin, bool out);
void gpio_set_function(unsigned pin, unsigned function);
void gpio_pull_up(unsigned pin);
void gpio_pull_down(unsigned pin);
void gpio_set_irq_enabled_with_callback(unsigned pin, uint32_t events, bool enabled, void (*callback)(unsigned,uint32_t));
void adc_init(void);
void adc_gpio_init(unsigned pin);
void adc_select_input(unsigned input);
uint16_t adc_read(void);
void i2c_init(void *bus, unsigned rate);
int i2c_write_timeout_us(void *bus, uint8_t addr, const uint8_t *bytes, size_t count, bool nostop, unsigned timeout);
int i2c_read_timeout_us(void *bus, uint8_t addr, uint8_t *bytes, size_t count, bool nostop, unsigned timeout);
unsigned pwm_gpio_to_slice_num(unsigned pin);
pwm_config pwm_get_default_config(void);
void pwm_config_set_clkdiv(pwm_config *cfg, float div);
void pwm_config_set_wrap(pwm_config *cfg, unsigned wrap);
void pwm_init(unsigned slice,const pwm_config *cfg,bool enabled);
void pwm_set_gpio_level(unsigned pin,uint16_t level);
void pwm_set_enabled(unsigned slice,bool enabled);
unsigned clock_get_hz(unsigned clock);
uint32_t save_and_disable_interrupts(void);
void restore_interrupts(uint32_t state);
#endif
