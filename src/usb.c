#include "usb.h"

#include "backlight.h"
#include "keyboard.h"
#include "platform.h"
#include "touchpad.h"
#include "reg.h"

static struct
{
	bool mouse_moved;
	uint8_t mouse_btn;
} self;

// TODO: What about Ctrl?
// TODO: What should L1, L2, R1, R2 do
// TODO: Should touch send arrow keys as an option?

static void key_cb(char key, enum key_state state)
{
	// Don't send mods over USB
	if ((key == KEY_MOD_SHL) ||
		(key == KEY_MOD_SHR) ||
		(key == KEY_MOD_ALT) ||
		(key == KEY_MOD_SYM))
		return;

	if (reg_is_bit_set(REG_ID_CF2, CF2_USB_KEYB_ON)) {
		platform_usb_keyboard_report(key, state == KEY_STATE_PRESSED, state == KEY_STATE_HOLD);
	}

	if (reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON)) {
		if (key == KEY_JOY_CENTER) {
			if (state == KEY_STATE_PRESSED) {
				self.mouse_btn = PLATFORM_MOUSE_BTN_LEFT;
				self.mouse_moved = false;
				platform_usb_mouse_report(self.mouse_btn, 0, 0);
			} else if ((state == KEY_STATE_HOLD) && !self.mouse_moved) {
				self.mouse_btn = PLATFORM_MOUSE_BTN_RIGHT;
				platform_usb_mouse_report(self.mouse_btn, 0, 0);
			} else if (state == KEY_STATE_RELEASED) {
				self.mouse_btn = 0x00;
				platform_usb_mouse_report(0x00, 0, 0);
			}
		}
	}
}
static struct key_callback key_callback = { .func = key_cb };

static void touch_cb(int8_t x, int8_t y)
{
	if (!reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON))
		return;

	self.mouse_moved = true;

	platform_usb_mouse_report(self.mouse_btn, x, y);
}
static struct touch_callback touch_callback = { .func = touch_cb };

static void on_connected(void)
{
	// if USB connected, default to sending mods
	reg_set_value(REG_ID_CFG, reg_get_value(REG_ID_CFG) | CFG_REPORT_MODS);
}

void usb_init(void)
{
	platform_usb_set_on_connected(&on_connected);
	platform_usb_init();

	keyboard_add_key_callback(&key_callback);

	touchpad_add_touch_callback(&touch_callback);
}
