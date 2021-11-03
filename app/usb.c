#include "usb.h"

#include "keyboard.h"
#include "touchpad.h"
#include "reg.h"

#include <hardware/irq.h>
#include <pico/mutex.h>
#include <tusb.h>

#define USB_LOW_PRIORITY_IRQ	31
#define USB_TASK_INTERVAL_US	1000

static struct
{
	mutex_t mutex;
	bool mouse_moved;
	uint8_t mouse_btn;
} self;

// TODO: Should mods always be sent?
// TODO: What about Ctrl?
// TODO: What should L1, L2, R1, R2 do
// TODO: Should touch send arrow keys as an option?

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

static void key_cb(char key, enum key_state state)
{
	if (tud_hid_n_ready(USB_ITF_KEYBOARD) && reg_is_bit_set(REG_ID_CF2, CF2_USB_KEYB_ON)) {
		uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

		uint8_t keycode[6] = { 0 };
		uint8_t modifier   = 0;

		if (state == KEY_STATE_PRESSED) {
			if (conv_table[(int)key][0])
				modifier = KEYBOARD_MODIFIER_LEFTSHIFT;

			keycode[0] = conv_table[(int)key][1];

			// Fixup: Enter instead of Return
			if (key == '\n')
				keycode[0] = HID_KEY_ENTER;
		}

		tud_hid_n_keyboard_report(USB_ITF_KEYBOARD, 0, modifier, keycode);
	}

	if (tud_hid_n_ready(USB_ITF_MOUSE) && reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON)) {
		if (key == KEY_JOY_CENTER) {
			if (state == KEY_STATE_PRESSED) {
				self.mouse_btn = MOUSE_BUTTON_LEFT;
				self.mouse_moved = false;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, MOUSE_BUTTON_LEFT, 0, 0, 0, 0);
			} else if ((state == KEY_STATE_HOLD) && !self.mouse_moved) {
				self.mouse_btn = MOUSE_BUTTON_RIGHT;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, MOUSE_BUTTON_RIGHT, 0, 0, 0, 0);
			} else if (state == KEY_STATE_RELEASED) {
				self.mouse_btn = 0x00;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, 0x00, 0, 0, 0, 0);
			}
		}
	}
}
static struct key_callback key_callback =
{
	.func = key_cb
};

static void touch_cb(int8_t x, int8_t y)
{
	if (!tud_hid_n_ready(USB_ITF_MOUSE) || !reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON))
		return;

	self.mouse_moved = true;

	tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, self.mouse_btn, x, y, 0, 0);
}
static struct touch_callback touch_callback =
{
	.func = touch_cb
};

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
	// TODO not Implemented
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	printf("%s: itf: %d, report id: %d, type: %d, len: %d\r\n", __func__, itf, report_id, report_type, reqlen);

	return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
	// TODO set LED based on CAPLOCK, NUMLOCK etc...
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)bufsize;

	printf("%s: itf: %d, report id: %d, type: %d, size: %d\r\n", __func__, itf, report_id, report_type, bufsize);
}

void usb_init(void)
{
	tusb_init();

	keyboard_add_key_callback(&key_callback);

	touchpad_add_touch_callback(&touch_callback);

	// create a new interrupt to call tud_task, and trigger that irq from a timer
	irq_set_exclusive_handler(USB_LOW_PRIORITY_IRQ, low_priority_worker_irq);
	irq_set_enabled(USB_LOW_PRIORITY_IRQ, true);

	mutex_init(&self.mutex);
	add_alarm_in_us(USB_TASK_INTERVAL_US, timer_task, NULL, true);
}

mutex_t *usb_get_mutex(void)
{
	return &self.mutex;
}
