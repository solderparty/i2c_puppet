#ifdef PICO_BUILD

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/irq.h>

static i2c_inst_t *i2c_instances[2] = { i2c0, i2c1 };

static i2c_inst_t *i2c;
static void (*receive_handler)(void);
static void (*request_handler)(void);

#define REG_ID_INVALID		0x00

static void irq_handler(void)
{
	// the controller sent data
	if (i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_RX_FULL_BITS) {
		if (receive_handler) {
			receive_handler();
		}
	}

	// the controller requested data
	if (i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_RD_REQ_BITS) {
		if (request_handler) {
			request_handler();
		}

		i2c->hw->clr_rd_req;
	}
}
uint8_t platform_puppet_i2c_read_byte(void)
{
	return i2c->hw->data_cmd & 0xff;
}

void platform_puppet_i2c_write(uint8_t *data, size_t len)
{
	i2c_write_raw_blocking(i2c, data, len);
}

void platform_puppet_i2c_set_address(uint8_t address)
{
	i2c_set_slave_mode(i2c, true, address);
}

void platform_puppet_i2c_init(uint8_t address, uint32_t sda_pin, uint32_t scl_pin, void (*recv_handler)(void), void (*req_handler)(void))
{
	// determine the instance based on the SCL pin, hope we didn't screw up the SDA pin!
	i2c = i2c_instances[(scl_pin / 2) % 2];

	i2c_init(i2c, 100 * 1000);
	i2c_set_slave_mode(i2c, true, address);

	gpio_set_function(sda_pin, GPIO_FUNC_I2C);
	gpio_pull_up(sda_pin);

	gpio_set_function(scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(scl_pin);

	receive_handler = recv_handler;
	request_handler = req_handler;

	// irq when the controller sends data, and when it requests a read
	i2c->hw->intr_mask = I2C_IC_INTR_MASK_M_RD_REQ_BITS | I2C_IC_INTR_MASK_M_RX_FULL_BITS;

	const int irq = I2C0_IRQ + i2c_hw_index(i2c);
	irq_set_exclusive_handler(irq, irq_handler);
	irq_set_enabled(irq, true);
}

#endif
