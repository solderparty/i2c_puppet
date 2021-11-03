#include "touchpad.h"

#include "keyboard.h"

#include <hardware/i2c.h>
#include <pico/binary_info.h>
#include <pico/stdlib.h>
#include <stdio.h>

#define DEV_ADDR			0x3B

#define REG_PID				0x00
#define REG_REV				0x01
#define REG_MOTION			0x02
#define REG_DELTA_X			0x03
#define REG_DELTA_Y			0x04
#define REG_DELTA_XY_H		0x05
#define REG_CONFIG			0x11
#define REG_OBSERV			0x2E
#define REG_MBURST			0x42

#define BIT_MOTION_MOT		(1 << 7)
#define BIT_MOTION_OVF		(1 << 4)

#define BIT_CONFIG_HIRES	(1 << 7)

#define BIT_OBSERV_RUN		(0 << 6)
#define BIT_OBSERV_REST1	(1 << 6)
#define BIT_OBSERV_REST2	(2 << 6)
#define BIT_OBSERV_REST3	(3 << 6)

#define SWIPE_COOLDOWN_TIME_MS	100 // time to wait before generating a new swipe event
#define SWIPE_RELEASE_DELAY_MS	10  // time to wait before sending key release event
#define MOTION_IS_SWIPE(i, j)	(((i >= 15) || (i <= -15)) && ((j >= -5) && (j <= 5)))

static i2c_inst_t *i2c_instances[2] = { i2c0, i2c1 };

static struct
{
	struct touch_callback *callbacks;
	uint32_t last_swipe_time;
	i2c_inst_t *i2c;
} self;

static uint8_t read_register8(uint8_t reg)
{
	uint8_t val;

	i2c_write_blocking(self.i2c, DEV_ADDR, &reg, sizeof(reg), true);
	i2c_read_blocking(self.i2c, DEV_ADDR, &val, sizeof(val), false);

	return val;
}

//static void write_register8(uint8_t reg, uint8_t val)
//{
//	uint8_t buffer[2] = { reg, val };
//	i2c_write_blocking(self.i2c, DEV_ADDR, buffer, sizeof(buffer), false);
//}

int64_t release_key(alarm_id_t id, void *user_data)
{
	(void)id;

	const int data = (int)user_data;

	keyboard_inject_event((char)data, KEY_STATE_RELEASED);

	return 0;
}

void touchpad_gpio_irq(uint gpio, uint32_t events)
{
	if (gpio != PIN_TP_MOTION)
		return;

	if (!(events & GPIO_IRQ_EDGE_FALL))
		return;

	const uint8_t motion = read_register8(REG_MOTION);
	if (motion & BIT_MOTION_MOT) {
		int8_t x = read_register8(REG_DELTA_X);
		int8_t y = read_register8(REG_DELTA_Y);

		x = ((x < 127) ? x : (x - 256)) * -1;
		y = ((y < 127) ? y : (y - 256));

		if (keyboard_is_mod_on(KEY_MOD_ID_ALT)) {
			if (to_ms_since_boot(get_absolute_time()) - self.last_swipe_time > SWIPE_COOLDOWN_TIME_MS) {
				char key = '\0';
				if (MOTION_IS_SWIPE(y, x)) {
					key = (y < 0) ? KEY_JOY_UP : KEY_JOY_DOWN;
				} else if (MOTION_IS_SWIPE(x, y)) {
					key = (x < 0) ? KEY_JOY_LEFT : KEY_JOY_RIGHT;
				}

				if (key != '\0') {
					keyboard_inject_event(key, KEY_STATE_PRESSED);

					// we need to allow the usb a bit of time to send the press, so schedule the release after a bit
					add_alarm_in_ms(SWIPE_RELEASE_DELAY_MS, release_key, (void*)(int)key, true);

					self.last_swipe_time = to_ms_since_boot(get_absolute_time());
				}
			}
		} else {
			if (self.callbacks) {
				struct touch_callback *cb = self.callbacks;

				while (cb) {
					cb->func(x, y);

					cb = cb->next;
				}
			}
		}
	}
}

void touchpad_add_touch_callback(struct touch_callback *callback)
{
	// first callback
	if (!self.callbacks) {
		self.callbacks = callback;
		return;
	}

	// find last and insert after
	struct touch_callback *cb = self.callbacks;
	while (cb->next)
		cb = cb->next;

	cb->next = callback;
}

void touchpad_init(void)
{
	// determine the instance based on SCL pin, hope you didn't screw up the SDA pin!
	self.i2c = i2c_instances[(PIN_SCL / 2) % 2];

	i2c_init(self.i2c, 100 * 1000);

	gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SDA);

	gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SCL);

	// Make the I2C pins available to picotool
	bi_decl(bi_2pins_with_func(PIN_SDA, PIN_SCL, GPIO_FUNC_I2C));

	gpio_init(PIN_TP_SHUTDOWN);
	gpio_set_dir(PIN_TP_SHUTDOWN, GPIO_OUT);
	gpio_put(PIN_TP_SHUTDOWN, 0);

	gpio_init(PIN_TP_MOTION);
	gpio_set_dir(PIN_TP_MOTION, GPIO_IN);
	gpio_set_irq_enabled(PIN_TP_MOTION, GPIO_IRQ_EDGE_FALL, true);

	gpio_init(PIN_TP_RESET);
	gpio_set_dir(PIN_TP_RESET, GPIO_OUT);

	gpio_put(PIN_TP_RESET, 0);
	sleep_ms(100);
	gpio_put(PIN_TP_RESET, 1);
}
