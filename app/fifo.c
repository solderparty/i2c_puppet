#include "app_config.h"
#include "fifo.h"

static struct
{
	struct fifo_item fifo[KEY_FIFO_SIZE];
	uint8_t count;
	uint8_t read_idx;
	uint8_t write_idx;
} self;

uint8_t fifo_count(void)
{
	return self.count;
}

void fifo_flush(void)
{
	self.write_idx = 0;
	self.read_idx = 0;
	self.count = 0;
}

bool fifo_enqueue(const struct fifo_item item)
{
	if (self.count >= KEY_FIFO_SIZE)
		return false;

	self.fifo[self.write_idx++] = item;

	self.write_idx %= KEY_FIFO_SIZE;
	++self.count;

	return true;
}

void fifo_enqueue_force(const struct fifo_item item)
{
	if (fifo_enqueue(item))
		return;

	self.fifo[self.write_idx++] = item;
	self.write_idx %= KEY_FIFO_SIZE;

	self.read_idx++;
	self.read_idx %= KEY_FIFO_SIZE;
}

struct fifo_item fifo_dequeue(void)
{
	struct fifo_item item = { 0 };
	if (self.count == 0)
		return item;

	item = self.fifo[self.read_idx++];
	self.read_idx %= KEY_FIFO_SIZE;
	--self.count;

	return item;
}
