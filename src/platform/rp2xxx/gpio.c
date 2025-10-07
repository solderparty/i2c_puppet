#ifdef PICO_BUILD

#include "platform.h"

#include <hardware/gpio.h>

static struct {
	struct platform_gpio_callback *callbacks;
	bool irq_callback_set;
} self;

static void gpio_irq(uint gpio, uint32_t events)
{
	//printf("gpio %s: gpio %d, events 0x%02X\r\n", __func__, gpio, events);

	struct platform_gpio_callback *cb = self.callbacks;
	while (cb) {
		if (cb->pin == gpio)
			cb->func(gpio, events);

		cb = cb->next;
	}
}

bool platform_gpio_get(uint32_t pin)
{
	return gpio_get(pin);
}

void platform_gpio_set(uint32_t pin, bool value)
{
	gpio_put(pin, value);
}

void platform_gpio_configure(uint32_t pin, enum platform_gpio_dir dir)
{
	gpio_init(pin);
	gpio_set_dir(pin, dir == PLATFORM_GPIO_OUTPUT);
}

void platform_gpio_attach_callback(uint32_t pin, uint32_t event_mask, struct platform_gpio_callback *callback)
{
	//printf("%s: pin: %d, event_mask: 0x%02X\r\n", __func__, pin, event_mask);

	callback->pin = pin;

	// first callback
	if (!self.callbacks) {
		self.callbacks = callback;
	} else {
		// find last and insert after
		struct platform_gpio_callback *cb = self.callbacks;
		while (cb->next)
			cb = cb->next;

		cb->next = callback;
	}

	if (!self.irq_callback_set) {
		gpio_set_irq_enabled_with_callback(0xff, 0, true, &gpio_irq);
		self.irq_callback_set = true;
	}

	gpio_set_irq_enabled(pin, event_mask, true);
}

#endif
