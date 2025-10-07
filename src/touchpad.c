#include "touchpad.h"

#include "keyboard.h"
#include "platform.h"
#include "variant.h"

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

static struct
{
	struct touch_callback *callbacks;
	uint32_t last_swipe_time;
} self;

static uint8_t read_register8(uint8_t reg)
{
	uint8_t val;

	platform_i2c_write(DEV_ADDR, &reg, sizeof(reg), true);
	platform_i2c_read(DEV_ADDR, &val, sizeof(val), false);

	return val;
}

static int32_t release_key(void *ctx)
{
	const int data = (int)ctx;

	keyboard_inject_event((char)data, KEY_STATE_RELEASED);

	return 0;
}
static struct platform_scheduled_callback release_key_callback = { .func = release_key };

static void gpio_cb(uint32_t gpio, uint32_t events)
{
	if (gpio != PIN_TP_MOTION)
		return;

	if (!(events & PLATFORM_GPIO_EVENT_EDGE_FALL))
		return;

	const uint8_t motion = read_register8(REG_MOTION);
	if (motion & BIT_MOTION_MOT) {
		int8_t x = read_register8(REG_DELTA_X);
		int8_t y = read_register8(REG_DELTA_Y);

		x = ((x < 127) ? x : (x - 256)) * -1;
		y = ((y < 127) ? y : (y - 256));

		if (keyboard_is_mod_on(KEY_MOD_ID_ALT)) {
			if (platform_millis() - self.last_swipe_time > SWIPE_COOLDOWN_TIME_MS) {
				char key = '\0';
				if (MOTION_IS_SWIPE(y, x)) {
					key = (y < 0) ? KEY_JOY_UP : KEY_JOY_DOWN;
				} else if (MOTION_IS_SWIPE(x, y)) {
					key = (x < 0) ? KEY_JOY_LEFT : KEY_JOY_RIGHT;
				}

				if (key != '\0') {
					keyboard_inject_event(key, KEY_STATE_PRESSED);

					// we need to allow the usb a bit of time to send the press, so schedule the release after a bit
					platform_schedule_callback(SWIPE_RELEASE_DELAY_MS, &release_key_callback, (void*)(int)key);

					self.last_swipe_time = platform_millis();
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
static struct platform_gpio_callback gpio_callback = { .func = gpio_cb };

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
	platform_i2c_init();

	platform_gpio_configure(PIN_TP_SHUTDOWN, PLATFORM_GPIO_OUTPUT);
	platform_gpio_set(PIN_TP_SHUTDOWN, 0);

	platform_gpio_configure(PIN_TP_MOTION, PLATFORM_GPIO_INPUT);
	platform_gpio_attach_callback(PIN_TP_MOTION, PLATFORM_GPIO_EVENT_EDGE_FALL, &gpio_callback);

	platform_gpio_configure(PIN_TP_RESET, PLATFORM_GPIO_OUTPUT);

	platform_gpio_set(PIN_TP_RESET, 0);
	platform_sleep_ms(100);
	platform_gpio_set(PIN_TP_RESET, 1);
}
