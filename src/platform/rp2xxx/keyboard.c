#ifdef PICO_BUILD

#include <hardware/gpio.h>

#include "reg.h"

void platform_keyboard_set_col(uint32_t pin)
{
	gpio_pull_up(pin);
	gpio_put(pin, 0);
	gpio_set_dir(pin, GPIO_OUT);
}

void platform_keyboard_clear_col(uint32_t pin)
{
	gpio_put(pin, 1);
	gpio_disable_pulls(pin);
	gpio_set_dir(pin, GPIO_IN);
}

void platform_keyboard_init(const uint32_t *row_pins, size_t row_count, const uint32_t *col_pins, size_t col_count, const uint32_t *btn_pins, size_t btn_count)
{
	for (uint32_t i = 0; i < row_count; ++i) {
		gpio_init(row_pins[i]);
		gpio_pull_up(row_pins[i]);
		gpio_set_dir(row_pins[i], GPIO_IN);
	}

	for(uint32_t i = 0; i < col_count; ++i) {
		gpio_init(col_pins[i]);
		gpio_set_dir(col_pins[i], GPIO_IN);
	}

	for(uint32_t i = 0; i < btn_count; ++i) {
		gpio_init(btn_pins[i]);
		gpio_pull_up(btn_pins[i]);
		gpio_set_dir(btn_pins[i], GPIO_IN);
	}
}

#endif
