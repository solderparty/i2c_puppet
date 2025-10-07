#include "puppet_i2c.h"

#include "platform.h"
#include "reg.h"

#include <hardware/i2c.h>
#include <hardware/irq.h>
#include <pico/stdlib.h>

#define REG_ID_INVALID		0x00

static struct
{
	struct
	{
		uint8_t reg;
		uint8_t data;
	} read_buffer;

	uint8_t write_buffer[2];
	uint8_t write_len;
} self;


static void receive_handler(void)
{
	if (self.read_buffer.reg == REG_ID_INVALID) {
		self.read_buffer.reg = platform_puppet_i2c_read_byte();

		if (self.read_buffer.reg & PACKET_WRITE_MASK) {
			// it'sq a reg write, we need to wait for the second byte before we process
			return;
		}
	} else {
		self.read_buffer.data = platform_puppet_i2c_read_byte();
	}

	reg_process_packet(self.read_buffer.reg, self.read_buffer.data, self.write_buffer, &self.write_len);

	// ready for the next operation
	self.read_buffer.reg = REG_ID_INVALID;
}

static void request_handler(void)
{
	platform_puppet_i2c_write(self.write_buffer, self.write_len);
}

void puppet_i2c_sync_address(void)
{
	platform_puppet_i2c_set_address(reg_get_value(REG_ID_ADR));
}

void puppet_i2c_init(void)
{
	platform_puppet_i2c_init(reg_get_value(REG_ID_ADR), PIN_PUPPET_SDA, PIN_PUPPET_SCL, &receive_handler, &request_handler);
}
