#ifdef PICO_BUILD

#include "usb.h"

#include <pico/stdio/driver.h>
#include <tusb.h>

#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000

// copied from pico_stdio_usb in the SDK
static void usb_out_chars(const char *buf, int length)
{
	static uint64_t last_avail_time;
	uint32_t owner;

	if (!mutex_try_enter(platform_usb_get_mutex(), &owner)) {
		if (owner == get_core_num())
			return;

		mutex_enter_blocking(platform_usb_get_mutex());
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

	mutex_exit(platform_usb_get_mutex());
}
static struct stdio_driver stdio_usb =
{
	.out_chars = usb_out_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
	.crlf_enabled = PICO_STDIO_DEFAULT_CRLF
#endif
};

void platform_debug_init(void)
{
	stdio_init_all();

	stdio_set_driver_enabled(&stdio_usb, true);
}
#endif
