#ifdef PICO_BUILD

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <pico/binary_info.h>

static i2c_inst_t *i2c_instances[2] = { i2c0, i2c1 };

static i2c_inst_t *i2c;

int platform_i2c_write(uint8_t addr, const uint8_t *src, size_t len, bool nostop)
{
	return i2c_write_blocking(i2c, addr, src, len, nostop);
}

int platform_i2c_read(uint8_t addr, uint8_t *dst, size_t len, bool nostop)
{
	return i2c_read_blocking(i2c, addr, dst, len, nostop);
}

void platform_i2c_init(void)
{
	// determine the instance based on SCL pin, hope you didn't screw up the SDA pin!
	i2c = i2c_instances[(PIN_SCL / 2) % 2];

	i2c_init(i2c, 100 * 1000);

	gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SDA);

	gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_SCL);

	// Make the I2C pins available to picotool
	bi_decl(bi_2pins_with_func(PIN_SDA, PIN_SCL, GPIO_FUNC_I2C));
}

#endif
