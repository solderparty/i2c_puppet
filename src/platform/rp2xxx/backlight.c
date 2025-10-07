#ifdef PICO_BUILD

#include <hardware/gpio.h>
#include <hardware/pwm.h>

static uint32_t pin;

void platform_backlight_set(uint8_t val)
{
	pwm_set_gpio_level(pin, val * 0x80);
}

void platform_backlight_init(uint32_t bl_pin)
{
	pin = bl_pin;

	gpio_set_function(bl_pin, GPIO_FUNC_PWM);

	const uint slice_num = pwm_gpio_to_slice_num(bl_pin);

	pwm_config config = pwm_get_default_config();
	pwm_init(slice_num, &config, true);
}

#endif
