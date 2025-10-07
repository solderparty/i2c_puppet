#ifdef PICO_BUILD

// TODO: get rid of
#include "keyboard.h"
#include "reg.h"

#include <hardware/irq.h>
#include <pico/mutex.h>
#include <tusb.h>

#define USB_LOW_PRIORITY_IRQ	31
#define USB_TASK_INTERVAL_US	1000

static struct
{
	mutex_t mutex;

	void (*on_connected)(void);

	uint8_t write_buffer[2];
	uint8_t write_len;
} self;

static void low_priority_worker_irq(void)
{
	if (mutex_try_enter(&self.mutex, NULL)) {
		tud_task();

		mutex_exit(&self.mutex);
	}
}

static int64_t timer_task(alarm_id_t id, void *user_data)
{
	(void)id;
	(void)user_data;

	irq_set_pending(USB_LOW_PRIORITY_IRQ);

	return USB_TASK_INTERVAL_US;
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	// TODO not Implemented
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t len)
{
	// TODO set LED based on CAPLOCK, NUMLOCK etc...
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)len;
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
//	printf("%s: itf: %d, avail: %d\r\n", __func__, itf, tud_vendor_n_available(itf));

	uint8_t buff[64] = { 0 };
	tud_vendor_n_read(itf, buff, 64);
//	printf("%s: %02X %02X %02X\r\n", __func__, buff[0], buff[1], buff[2]);

	reg_process_packet(buff[0], buff[1], self.write_buffer, &self.write_len);

	tud_vendor_n_write(itf, self.write_buffer, self.write_len);
}

void tud_mount_cb(void)
{
	if (self.on_connected)
		self.on_connected();
}

mutex_t *platform_usb_get_mutex(void)
{
	return &self.mutex;
}

void platform_usb_keyboard_report(char key, bool pressed, bool is_hold)
{
	if (!tud_hid_n_ready(USB_ITF_KEYBOARD))
		return;

	uint8_t conv_table[128][2]		= { HID_ASCII_TO_KEYCODE };
	conv_table['\n'][1]				= HID_KEY_ENTER; // Fixup: Enter instead of Return
	conv_table[KEY_JOY_UP][1]		= HID_KEY_ARROW_UP;
	conv_table[KEY_JOY_DOWN][1]		= HID_KEY_ARROW_DOWN;
	conv_table[KEY_JOY_LEFT][1]		= HID_KEY_ARROW_LEFT;
	conv_table[KEY_JOY_RIGHT][1]	= HID_KEY_ARROW_RIGHT;

	uint8_t keycode[6] = { 0 };
	uint8_t modifier   = 0;

	if (pressed) {
		if (conv_table[(int)key][0])
			modifier = KEYBOARD_MODIFIER_LEFTSHIFT;

		keycode[0] = conv_table[(int)key][1];
	}

	if (!is_hold)
		tud_hid_n_keyboard_report(USB_ITF_KEYBOARD, 0, modifier, keycode);
}

void platform_usb_mouse_report(uint8_t buttons, int8_t x, int8_t y)
{
	if (!tud_hid_n_ready(USB_ITF_MOUSE))
		return;

	tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, buttons, x, y, 0, 0);
}

void platform_usb_set_on_connected(void (*func)(void))
{
	self.on_connected = func;
}

void platform_usb_init(void)
{
	tusb_init();

	// create a new interrupt that calls tud_task, and trigger that interrupt from a timer
	irq_set_exclusive_handler(USB_LOW_PRIORITY_IRQ, low_priority_worker_irq);
	irq_set_enabled(USB_LOW_PRIORITY_IRQ, true);

	mutex_init(&self.mutex);
	add_alarm_in_us(USB_TASK_INTERVAL_US, timer_task, NULL, true);
}

#endif
