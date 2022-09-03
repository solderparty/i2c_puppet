#include "backlight.h"
#include "reg.h"

#include <hardware/pwm.h>
#include <pico/stdlib.h>

#define BACKLIGHT_IDLE_POLLING_RATE_MS 500

static struct {
	uint64_t last_triggered;
	bool idling;
	bool alarm_is_set;
	uint8_t backlight_level;
} self;

void backlight_sync(void)
{
	pwm_set_gpio_level(PIN_BKL, self.backlight_level  * 0x80);
}

static int64_t idle_detector_timer_task(alarm_id_t id, void *user_data)
{
	(void)id;
	(void)user_data;

	uint8_t current_dimming_delay = reg_get_value(REG_ID_BK3);
	if (current_dimming_delay == 0) {
		self.idling = true;
		cancel_alarm(id);
		self.alarm_is_set = false;
	} else if (!self.idling && to_ms_since_boot(get_absolute_time()) - self.last_triggered > (current_dimming_delay * 500)) {
		self.idling = true;
		cancel_alarm(id);
		self.alarm_is_set = false;
		self.backlight_level = reg_get_value(REG_ID_BK4);
		backlight_sync();
	}
	return BACKLIGHT_IDLE_POLLING_RATE_MS * 1000;
}

void backlight_trigger(void)
{
	self.idling = false;
	self.last_triggered = to_ms_since_boot(get_absolute_time());
	uint8_t desired_brightness = reg_get_value(REG_ID_BKL);
	if (self.backlight_level < desired_brightness) {
		self.backlight_level = desired_brightness;
		backlight_sync();
	}
	if (!self.alarm_is_set) {
		add_alarm_in_ms(BACKLIGHT_IDLE_POLLING_RATE_MS, idle_detector_timer_task, NULL, true);
		self.alarm_is_set = true;
	}
}

void backlight_init(void)
{
	self.idling = false;
	gpio_set_function(PIN_BKL, GPIO_FUNC_PWM);

	const uint slice_num = pwm_gpio_to_slice_num(PIN_BKL);

	pwm_config config = pwm_get_default_config();
	pwm_init(slice_num, &config, true);

 	self.backlight_level = reg_get_value(REG_ID_BKL);
	backlight_sync();
	self.last_triggered = 0;

	if (reg_get_value(REG_ID_BK3) > 0) {
		add_alarm_in_ms(BACKLIGHT_IDLE_POLLING_RATE_MS, idle_detector_timer_task, NULL, true);
		self.alarm_is_set = true;
	}
}
