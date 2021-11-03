#include "puppet_i2c.h"

#include "app_config.h"
#include "backlight.h"
#include "fifo.h"
#include "gpioexp.h"
#include "keyboard.h"
#include "reg.h"

#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>
#include <RP2040.h>
#include <stdio.h>

#define WRITE_MASK			(1 << 7)
#define REG_ID_INVALID		0x00

static i2c_inst_t *i2c_instances[2] = { i2c0, i2c1 };

static struct
{
	i2c_inst_t *i2c;

	struct
	{
		uint8_t reg;
		uint8_t data;
	} read_buffer;

	uint8_t write_buffer[2];
	uint8_t write_len;
} self;

static void process_read(void)
{
	const bool is_write = (self.read_buffer.reg & WRITE_MASK);
	const uint8_t reg = (self.read_buffer.reg & ~WRITE_MASK);

//	printf("read complete, is_write: %d, reg: 0x%02X\r\n", is_write, reg);

	switch (reg) {

	// common R/W registers
	case REG_ID_CFG:
	case REG_ID_INT:
	case REG_ID_DEB:
	case REG_ID_FRQ:
	case REG_ID_BKL:
	case REG_ID_BK2:
	case REG_ID_GIC:
	case REG_ID_GIN:
	case REG_ID_HLD:
	case REG_ID_ADR:
	case REG_ID_IND:
	case REG_ID_CF2:
	{
		if (is_write) {
			reg_set_value(reg, self.read_buffer.data);

			switch (reg) {
			case REG_ID_BKL:
			case REG_ID_BK2:
				backlight_sync();
				break;

			case REG_ID_ADR:
				puppet_i2c_sync_address();
				break;

			default:
				break;
			}
		} else {
			self.write_buffer[0] = reg_get_value(reg);
			self.write_len = sizeof(uint8_t);
		}
		break;
	}

	// special R/W registers
	case REG_ID_DIR: // gpio direction
	case REG_ID_PUE: // gpio input pull enable
	case REG_ID_PUD: // gpio input pull direction
	{
		if (is_write) {
			switch (reg) {
			case REG_ID_DIR:
				gpioexp_update_dir(self.read_buffer.data);
				break;
			case REG_ID_PUE:
				gpioexp_update_pue_pud(self.read_buffer.data, reg_get_value(REG_ID_PUD));
				break;
			case REG_ID_PUD:
				gpioexp_update_pue_pud(reg_get_value(REG_ID_PUE), self.read_buffer.data);
				break;
			}
		} else {
			self.write_buffer[0] = reg_get_value(reg);
			self.write_len = sizeof(uint8_t);
		}
		break;
	}

	case REG_ID_GIO: // gpio value
	{
		if (is_write) {
			gpioexp_set_value(self.read_buffer.data);
		} else {
			self.write_buffer[0] = gpioexp_get_value();
			self.write_len = sizeof(uint8_t);
		}
		break;
	}

	// read-only registers
	case REG_ID_TOX:
	case REG_ID_TOY:
		self.write_buffer[0] = reg_get_value(reg);
		self.write_len = sizeof(uint8_t);

		reg_set_value(reg, 0);
		break;

	case REG_ID_VER:
		self.write_buffer[0] = VER_VAL;
		self.write_len = sizeof(uint8_t);
		break;

	case REG_ID_KEY:
		self.write_buffer[0] = fifo_count();
		self.write_buffer[0] |= keyboard_get_numlock()  ? KEY_NUMLOCK  : 0x00;
		self.write_buffer[0] |= keyboard_get_capslock() ? KEY_CAPSLOCK : 0x00;
		self.write_len = sizeof(uint8_t);
		break;

	case REG_ID_FIF:
	{
		const struct fifo_item item = fifo_dequeue();

		self.write_buffer[0] = (uint8_t)item.state;
		self.write_buffer[1] = (uint8_t)item.key;
		self.write_len = sizeof(uint8_t) * 2;
		break;
	}

	case REG_ID_RST:
		NVIC_SystemReset();
		break;
	}
}

static void irq_handler(void)
{
	// the controller sent data
	if (self.i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_RX_FULL_BITS) {
		if (self.read_buffer.reg == REG_ID_INVALID) {
			self.read_buffer.reg = self.i2c->hw->data_cmd & 0xff;

			if (self.read_buffer.reg & WRITE_MASK) {
				// it'sq a reg write, we need to wait for the second byte before we process
				return;
			}
		} else {
			self.read_buffer.data = self.i2c->hw->data_cmd & 0xff;
		}

		process_read();

		// ready for the next operation
		self.read_buffer.reg = REG_ID_INVALID;

		return;
	}

	// the controller requested a read
	if (self.i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_RD_REQ_BITS) {
		i2c_write_raw_blocking(self.i2c, self.write_buffer, self.write_len);

		self.i2c->hw->clr_rd_req;
		return;
	}
}

void puppet_i2c_sync_address(void)
{
	i2c_set_slave_mode(self.i2c, true, reg_get_value(REG_ID_ADR));
}

void puppet_i2c_init(void)
{
	// determine the instance based on SCL pin, hope you didn't screw up the SDA pin!
	self.i2c = i2c_instances[(PIN_PUPPET_SCL / 2) % 2];

	i2c_init(self.i2c, 100 * 1000);
	puppet_i2c_sync_address();

	gpio_set_function(PIN_PUPPET_SDA, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_PUPPET_SDA);

	gpio_set_function(PIN_PUPPET_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(PIN_PUPPET_SCL);

	// irq when the controller sends data, and when it requests a read
	self.i2c->hw->intr_mask = I2C_IC_INTR_MASK_M_RD_REQ_BITS | I2C_IC_INTR_MASK_M_RX_FULL_BITS;

	const int irq = I2C0_IRQ + i2c_hw_index(self.i2c);
	irq_set_exclusive_handler(irq, irq_handler);
	irq_set_enabled(irq, true);
}
