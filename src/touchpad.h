#pragma once

#include <stdbool.h>
#include <sys/types.h>

struct touch_callback
{
	void (*func)(int8_t x, int8_t y);
	struct touch_callback *next;
};

void touchpad_add_touch_callback(struct touch_callback *callback);

void touchpad_init(void);
