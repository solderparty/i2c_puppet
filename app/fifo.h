#pragma once

#include "keyboard.h"

struct fifo_item
{
	char key;
	enum key_state state;
};

uint8_t fifo_count(void);
void fifo_flush(void);
bool fifo_enqueue(const struct fifo_item item);
void fifo_enqueue_force(const struct fifo_item item);
struct fifo_item fifo_dequeue(void);
