#include "debug.h"

#include "app_config.h"
#include "gpioexp.h"
#include "keyboard.h"
#include "reg.h"
#include "touchpad.h"
#include "usb.h"

#include <pico/mutex.h>
#include <pico/stdio/driver.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <tusb.h>

#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000

static void key_cb(char key, enum key_state state)
{
	printf("key: 0x%02X/%d/%c, state: %d\r\n", key, key, key, state);
}
static struct key_callback key_callback = { .func = key_cb };

static void key_lock_cb(bool caps_changed, bool num_changed)
{
	printf("lock, caps_c: %d, caps: %d, num_c: %d, num: %d\r\n",
		   caps_changed, keyboard_get_capslock(),
		   num_changed, keyboard_get_numlock());
}
static struct key_lock_callback key_lock_callback ={ .func = key_lock_cb };

static void touch_cb(int8_t x, int8_t y)
{
	printf("%s: x: %d, y: %d !\r\n", __func__, x, y);
}
static struct touch_callback touch_callback = { .func = touch_cb };

static void gpioexp_cb(uint8_t gpio, uint8_t gpio_idx)
{
	printf("gpioexp, pin: %d, idx: %d\r\n", gpio, gpio_idx);
}
static struct gpioexp_callback gpioexp_callback = { .func = gpioexp_cb };

// copied from pico_stdio_usb in the SDK
static void usb_out_chars(const char *buf, int length)
{
	static uint64_t last_avail_time;
	uint32_t owner;

	if (!mutex_try_enter(usb_get_mutex(), &owner)) {
		if (owner == get_core_num())
			return;

		mutex_enter_blocking(usb_get_mutex());
	}

	if (tud_cdc_connected()) {
		for (int i = 0; i < length;) {
			int n = length - i;
			int avail = tud_cdc_write_available();
			if (n > avail) n = avail;
			if (n) {
				int n2 = tud_cdc_write(buf + i, n);
				tud_task();
				tud_cdc_write_flush();
				i += n2;
				last_avail_time = time_us_64();
			} else {
				tud_task();
				tud_cdc_write_flush();
				if (!tud_cdc_connected() ||
					(!tud_cdc_write_available() && time_us_64() > last_avail_time + PICO_STDIO_USB_STDOUT_TIMEOUT_US)) {
					break;
				}
			}
		}
	} else {
		// reset our timeout
		last_avail_time = 0;
	}

	mutex_exit(usb_get_mutex());
}
static struct stdio_driver stdio_usb =
{
	.out_chars = usb_out_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
	.crlf_enabled = PICO_STDIO_DEFAULT_CRLF
#endif
};

void debug_init(void)
{
	stdio_init_all();

	stdio_set_driver_enabled(&stdio_usb, true);

	printf("I2C Puppet SW v%d.%d\r\n", VERSION_MAJOR, VERSION_MINOR);

	keyboard_add_key_callback(&key_callback);
	keyboard_add_lock_callback(&key_lock_callback);

	touchpad_add_touch_callback(&touch_callback);

	gpioexp_add_int_callback(&gpioexp_callback);
}
