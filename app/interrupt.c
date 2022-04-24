#include "interrupt.h"

#include "app_config.h"
#include "gpioexp.h"
#include "keyboard.h"
#include "reg.h"
#include "touchpad.h"

#include <pico/stdlib.h>

static void key_cb(char key, enum key_state state)
{
	(void)key;
	(void)state;

	if (!reg_is_bit_set(REG_ID_CFG, CFG_KEY_INT))
		return;

	reg_set_bit(REG_ID_INT, INT_KEY);

	gpio_put(PIN_INT, 0);
	busy_wait_ms(reg_get_value(REG_ID_IND));
	gpio_put(PIN_INT, 1);
}
static struct key_callback key_callback = { .func = key_cb };

static void key_lock_cb(bool caps_changed, bool num_changed)
{
	bool do_int = false;

	if (caps_changed && reg_is_bit_set(REG_ID_CFG, CFG_CAPSLOCK_INT)) {
		reg_set_bit(REG_ID_INT, INT_CAPSLOCK);
		do_int = true;
	}

	if (num_changed && reg_is_bit_set(REG_ID_CFG, CFG_NUMLOCK_INT)) {
		reg_set_bit(REG_ID_INT, INT_NUMLOCK);
		do_int = true;
	}

	if (do_int) {
		gpio_put(PIN_INT, 0);
		busy_wait_ms(reg_get_value(REG_ID_IND));
		gpio_put(PIN_INT, 1);
	}
}
static struct key_lock_callback key_lock_callback = { .func = key_lock_cb };

static void touch_cb(int8_t x, int8_t y)
{
	(void)x;
	(void)y;

	if (!reg_is_bit_set(REG_ID_CF2, CF2_TOUCH_INT))
		return;

	reg_set_bit(REG_ID_INT, INT_TOUCH);

	gpio_put(PIN_INT, 0);
	busy_wait_ms(reg_get_value(REG_ID_IND));
	gpio_put(PIN_INT, 1);
}
static struct touch_callback touch_callback = { .func = touch_cb };

static void gpioexp_cb(uint8_t gpio, uint8_t gpio_idx)
{
	(void)gpio;

	if (!reg_is_bit_set(REG_ID_GIC, (1 << gpio_idx)))
		return;

	reg_set_bit(REG_ID_INT, INT_GPIO);
	reg_set_bit(REG_ID_GIN, (1 << gpio_idx));

	gpio_put(PIN_INT, 0);
	busy_wait_ms(reg_get_value(REG_ID_IND));
	gpio_put(PIN_INT, 1);
}
static struct gpioexp_callback gpioexp_callback = { .func = gpioexp_cb };

void interrupt_init(void)
{
	gpio_init(PIN_INT);
	gpio_set_dir(PIN_INT, GPIO_OUT);
	gpio_pull_up(PIN_INT);
	gpio_put(PIN_INT, true);

	keyboard_add_key_callback(&key_callback);
	keyboard_add_lock_callback(&key_lock_callback);

	touchpad_add_touch_callback(&touch_callback);

	gpioexp_add_int_callback(&gpioexp_callback);
}
