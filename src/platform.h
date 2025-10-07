#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PLATFORM_MOUSE_BTN_LEFT				0x01
#define PLATFORM_MOUSE_BTN_RIGHT			0x02
#define PLATFORM_MOUSE_BTN_MIDDLE			0x04

#define PLATFORM_GPIO_EVENT_LEVEL_LOW		0x01
#define PLATFORM_GPIO_EVENT_LEVEL_HIGH		0x02
#define PLATFORM_GPIO_EVENT_EDGE_FALL		0x04
#define PLATFORM_GPIO_EVENT_EDGE_RISE		0x08

enum platform_gpio_dir
{
	PLATFORM_GPIO_INPUT		= 0,
	PLATFORM_GPIO_OUTPUT	= 1,
};

struct platform_gpio_callback
{
	void (*func)(uint32_t pin, uint32_t events);
	uint32_t pin;
	struct platform_gpio_callback *next;
};

struct platform_scheduled_callback
{
	int32_t (*func)(void *ctx);
	void *ctx;
};

void platform_backlight_set(uint8_t val);
void platform_backlight_init(uint32_t pin);

void platform_debug_init(void);

bool platform_gpio_get(uint32_t pin);
void platform_gpio_set(uint32_t pin, bool value);
void platform_gpio_configure(uint32_t pin, enum platform_gpio_dir dir);
void platform_gpio_attach_callback(uint32_t pin, uint32_t event_mask, struct platform_gpio_callback *callback);

void platform_gpioexp_configure(uint32_t pin, uint8_t gpio_idx, bool input, bool pull_en, bool pull_up);

int platform_i2c_write(uint8_t addr, const uint8_t *src, size_t len, bool nostop);
int platform_i2c_read(uint8_t addr, uint8_t *dst, size_t len, bool nostop);
void platform_i2c_init(void);

void platform_interrupt_fire(void);
void platform_interrupt_init(void);

void platform_keyboard_set_col(uint32_t pin);
void platform_keyboard_clear_col(uint32_t pin);
void platform_keyboard_init(const uint32_t *row_pins, size_t row_count, const uint32_t *col_pins, size_t col_count, const uint32_t *btn_pins, size_t btn_count);

void platform_puppet_i2c_set_address(uint8_t address);
uint8_t platform_puppet_i2c_read_byte(void);
void platform_puppet_i2c_write(uint8_t *data, size_t len);
void platform_puppet_i2c_init(uint8_t address, uint32_t sda_pin, uint32_t scl_pin, void (*receive_handler)(void), void (*request_handler)(void));

void platform_usb_keyboard_report(char key, bool pressed, bool is_hold);
void platform_usb_mouse_report(uint8_t buttons, int8_t x, int8_t y);
void platform_usb_set_on_connected(void (*func)(void));
void platform_usb_init(void);

uint32_t platform_millis(void);
void platform_schedule_callback(uint32_t ms, struct platform_scheduled_callback *callback, void *ctx);
void platform_sleep_ms(uint32_t ms);
void platform_reset(void);
