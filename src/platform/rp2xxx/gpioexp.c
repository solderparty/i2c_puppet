#ifdef PICO_BUILD

#include <hardware/gpio.h>

void platform_gpioexp_configure(uint32_t pin, uint8_t gpio_idx, bool input, bool pull_en, bool pull_up)
{
#ifndef NDEBUG
	printf("%s: pin: %d, gpio_idx: %d, input: %d, pull_en: %d, pull_up: %d\r\n", __func__, pin, gpio_idx, input, pull_en, pull_up);
#endif

	gpio_init(pin);

	if (input) {
		gpio_set_pulls(pin, pull_en && pull_up, pull_en && !pull_up);

		gpio_set_dir(pin, GPIO_IN);

		gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
	} else {
		gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

		gpio_set_dir(pin, GPIO_OUT);
	}
}

#endif
