#pragma once

#include <stdbool.h>
#include <sys/types.h>

struct touch_callback
{
	void (*func)(int8_t, int8_t);
	struct touch_callback *next;
};

void touchpad_gpio_irq(uint gpio, uint32_t events);

void touchpad_add_touch_callback(struct touch_callback *callback);

void touchpad_init(void);
