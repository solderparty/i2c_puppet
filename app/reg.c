#include "reg.h"

#include "touchpad.h"

#include <stdio.h>

static struct
{
	uint8_t regs[REG_ID_LAST];
} self;

static void touch_cb(int8_t x, int8_t y)
{
	self.regs[REG_ID_TOX] = x;
	self.regs[REG_ID_TOY] = y;
}
static struct touch_callback touch_callback =
{
	.func = touch_cb
};

uint8_t reg_get_value(enum reg_id reg)
{
	return self.regs[reg];
}

void reg_set_value(enum reg_id reg, uint8_t value)
{
#ifndef NDEBUG
	printf("%s: reg: 0x%02X, val: 0x%02X\r\n", __func__, reg, value);
#endif

	self.regs[reg] = value;
}

bool reg_is_bit_set(enum reg_id reg, uint8_t bit)
{
	return self.regs[reg] & bit;
}

void reg_set_bit(enum reg_id reg, uint8_t bit)
{
#ifndef NDEBUG
	printf("%s: reg: 0x%02X, bit: %d\r\n", __func__, reg, bit);
#endif

	self.regs[reg] |= bit;
}

void reg_clear_bit(enum reg_id reg, uint8_t bit)
{
#ifndef NDEBUG
	printf("%s: reg: 0x%02X, bit: %d\r\n", __func__, reg, bit);
#endif

	self.regs[reg] &= ~bit;
}

void reg_init(void)
{
	self.regs[REG_ID_CFG] = CFG_OVERFLOW_INT | CFG_KEY_INT | CFG_USE_MODS;
	self.regs[REG_ID_BKL] = 255;
	self.regs[REG_ID_DEB] = 10;
	self.regs[REG_ID_FRQ] = 10; // ms
	self.regs[REG_ID_BK2] = 255;
	self.regs[REG_ID_PUD] = 0xFF;
	self.regs[REG_ID_HLD] = 30; // 10ms units
	self.regs[REG_ID_ADR] = 0x1F;
	self.regs[REG_ID_IND] = 1; // ms
	self.regs[REG_ID_CF2] = CF2_TOUCH_INT | CF2_USB_KEYB_ON | CF2_USB_MOUSE_ON;

	touchpad_add_touch_callback(&touch_callback);
}
