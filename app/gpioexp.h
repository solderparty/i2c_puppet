#pragma once

#include <sys/types.h>

struct gpioexp_callback
{
	void (*func)(uint8_t gpio, uint8_t gpio_idx);
	struct gpioexp_callback *next;
};

void gpioexp_gpio_irq(uint gpio, uint32_t events);

void gpioexp_update_dir(uint8_t dir);
void gpioexp_update_pue_pud(uint8_t pue, uint8_t pud);

void gpioexp_set_value(uint8_t value);
uint8_t gpioexp_get_value(void);

void gpioexp_add_int_callback(struct gpioexp_callback *callback);
void gpioexp_init(void);
