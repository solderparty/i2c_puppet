#include "backlight.h"
#include "debug.h"
#include "gpioexp.h"
#include "interrupt.h"
#include "keyboard.h"
#include "puppet_i2c.h"
#include "reg.h"
#include "touchpad.h"
#include "usb.h"

#include <stdio.h>

void setup(void)
{
	// The here order is important because it determines callback call order
	usb_init();

#ifndef NDEBUG
	debug_init();
#endif

	reg_init();

	backlight_init();

	gpioexp_init();

	keyboard_init();

	touchpad_init();

	interrupt_init();

	puppet_i2c_init();

#ifndef NDEBUG
	printf("Starting main loop\r\n");
#endif
}

void loop(void)
{
	asm volatile("wfe");
}
