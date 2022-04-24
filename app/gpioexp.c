#include "gpioexp.h"
#include "reg.h"

#include <pico/stdlib.h>
#include <stdio.h>

static struct
{
	struct gpioexp_callback *callbacks;
} self;

static void set_dir(uint8_t gpio, uint8_t gpio_idx, uint8_t dir)
{
#ifndef NDEBUG
	printf("%s: gpio: %d, gpio_idx: %d, dir: %d\r\n", __func__, gpio, gpio_idx, dir);
#endif

	gpio_init(gpio);

	if (dir == DIR_INPUT) {
		if (reg_is_bit_set(REG_ID_PUE, (1 << gpio_idx))) {
			if (reg_is_bit_set(REG_ID_PUD, (1 << gpio_idx)) == PUD_UP) {
				gpio_is_pulled_up(gpio);
			} else {
				gpio_is_pulled_down(gpio);
			}
		} else {
			gpio_disable_pulls(gpio);
		}

		gpio_set_dir(gpio, GPIO_IN);

		gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

		reg_set_bit(REG_ID_DIR, (1 << gpio_idx));
	} else {
		gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

		gpio_set_dir(gpio, GPIO_OUT);

		reg_clear_bit(REG_ID_DIR, (1 << gpio_idx));
	}
}

void gpioexp_gpio_irq(uint gpio, uint32_t events)
{
	(void)gpio;
	(void)events;

#define CALLBACK(bit) \
	if (gpio == PIN_GPIOEXP ## bit) { \
		struct gpioexp_callback *cb = self.callbacks; \
		while (cb) { \
			cb->func(PIN_GPIOEXP ## bit, bit); \
			cb = cb->next; \
		} \
		return; \
	}

#ifdef PIN_GPIOEXP0
	CALLBACK(0)
#endif

#ifdef PIN_GPIOEXP1
	CALLBACK(1)
#endif

#ifdef PIN_GPIOEXP2
	CALLBACK(2)
#endif

#ifdef PIN_GPIOEXP3
	CALLBACK(3)
#endif

#ifdef PIN_GPIOEXP4
	CALLBACK(4)
#endif

#ifdef PIN_GPIOEXP5
	CALLBACK(5)
#endif

#ifdef PIN_GPIOEXP6
	CALLBACK(6)
#endif

#ifdef PIN_GPIOEXP7
	CALLBACK(7)
#endif
}

void gpioexp_update_dir(uint8_t new_dir)
{
#ifndef NDEBUG
	printf("%s: dir: 0x%02X\r\n", __func__, new_dir);
#endif

	const uint8_t old_dir = reg_get_value(REG_ID_DIR);

	(void)old_dir; // Shut up warning in case no GPIOs configured

#define UPDATE_DIR(bit) \
	if ((old_dir & (1 << bit)) != (new_dir & (1 << bit))) \
		set_dir(PIN_GPIOEXP ## bit, bit, (new_dir & (1 << bit)) != 0);

#ifdef PIN_GPIOEXP0
	UPDATE_DIR(0)
#endif
#ifdef PIN_GPIOEXP1
	UPDATE_DIR(1)
#endif
#ifdef PIN_GPIOEXP2
	UPDATE_DIR(2)
#endif
#ifdef PIN_GPIOEXP3
	UPDATE_DIR(3)
#endif
#ifdef PIN_GPIOEXP4
	UPDATE_DIR(4)
#endif
#ifdef PIN_GPIOEXP5
	UPDATE_DIR(5)
#endif
#ifdef PIN_GPIOEXP6
	UPDATE_DIR(6)
#endif
#ifdef PIN_GPIOEXP7
	UPDATE_DIR(7)
#endif
}

void gpioexp_update_pue_pud(uint8_t new_pue, uint8_t new_pud)
{
#ifndef NDEBUG
	printf("%s: pue: 0x%02X, pud: 0x%02X\r\n", __func__, new_pue, new_pud);
#endif

	const uint8_t old_pue = reg_get_value(REG_ID_PUE);
	const uint8_t old_pud = reg_get_value(REG_ID_PUD);

	// Shut up warnings in case no GPIOs configured
	(void)old_pue;
	(void)old_pud;

	reg_set_value(REG_ID_PUE, new_pue);
	reg_set_value(REG_ID_PUD, new_pud);

#define UPDATE_PULL(bit) \
	if (((old_pue & (1 << bit)) != (new_pue & (1 << bit))) || \
		((old_pud & (1 << bit)) != (new_pud & (1 << bit)))) { \
		set_dir(PIN_GPIOEXP ## bit, bit, reg_is_bit_set(REG_ID_DIR, (1 << bit))); \
	}

#ifdef PIN_GPIOEXP0
	UPDATE_PULL(0)
#endif
#ifdef PIN_GPIOEXP1
	UPDATE_PULL(1)
#endif
#ifdef PIN_GPIOEXP2
	UPDATE_PULL(2)
#endif
#ifdef PIN_GPIOEXP3
	UPDATE_PULL(3)
#endif
#ifdef PIN_GPIOEXP4
	UPDATE_PULL(4)
#endif
#ifdef PIN_GPIOEXP5
	UPDATE_PULL(5)
#endif
#ifdef PIN_GPIOEXP6
	UPDATE_PULL(6)
#endif
#ifdef PIN_GPIOEXP7
	UPDATE_PULL(7)
#endif
}

void gpioexp_set_value(uint8_t value)
{
#ifndef NDEBUG
	printf("%s: value: 0x%02X\r\n", __func__, value);
#endif

#define SET_VALUE(bit) \
	if (reg_is_bit_set(REG_ID_DIR, (1 << bit)) == DIR_OUTPUT) { \
		gpio_put(PIN_GPIOEXP ## bit, (value & (1 << bit))); \
	}

#ifdef PIN_GPIOEXP0
	SET_VALUE(0)
#endif
#ifdef PIN_GPIOEXP1
	SET_VALUE(1)
#endif
#ifdef PIN_GPIOEXP2
	SET_VALUE(2)
#endif
#ifdef PIN_GPIOEXP3
	SET_VALUE(3)
#endif
#ifdef PIN_GPIOEXP4
	SET_VALUE(4)
#endif
#ifdef PIN_GPIOEXP5
	SET_VALUE(5)
#endif
#ifdef PIN_GPIOEXP6
	SET_VALUE(6)
#endif
#ifdef PIN_GPIOEXP7
	SET_VALUE(7)
#endif
}

uint8_t gpioexp_get_value(void)
{
	uint8_t value = 0;

#define GET_VALUE(bit) \
	value |= (gpio_get(PIN_GPIOEXP ## bit) << bit);

#ifdef PIN_GPIOEXP0
	GET_VALUE(0)
#endif
#ifdef PIN_GPIOEXP1
	GET_VALUE(1)
#endif
#ifdef PIN_GPIOEXP2
	GET_VALUE(2)
#endif
#ifdef PIN_GPIOEXP3
	GET_VALUE(3)
#endif
#ifdef PIN_GPIOEXP4
	GET_VALUE(4)
#endif
#ifdef PIN_GPIOEXP5
	GET_VALUE(5)
#endif
#ifdef PIN_GPIOEXP6
	GET_VALUE(6)
#endif
#ifdef PIN_GPIOEXP7
	GET_VALUE(7)
#endif

	return value;
}

void gpioexp_add_int_callback(struct gpioexp_callback *callback)
{
	// first callback
	if (!self.callbacks) {
		self.callbacks = callback;
		return;
	}

	// find last and insert after
	struct gpioexp_callback *cb = self.callbacks;
	while (cb->next)
		cb = cb->next;

	cb->next = callback;
}

void gpioexp_init(void)
{
	// Configure all to inputs
	gpioexp_update_dir(0xFF);
}
