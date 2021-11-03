#include "app_config.h"
#include "fifo.h"
#include "keyboard.h"
#include "reg.h"

#include <pico/stdlib.h>

#define LIST_SIZE	10 // size of the list keeping track of all the pressed keys

enum mod
{
	MOD_NONE = 0,
	MOD_SYM,
	MOD_ALT,
	MOD_SHL,
	MOD_SHR,

	MOD_LAST,
};

struct entry
{
	char chr;
	char symb;
	enum mod mod;
};

struct list_item
{
	const struct entry *p_entry;
	uint32_t hold_start_time;
	enum key_state state;
	bool mods[MOD_LAST];
};

static const uint8_t row_pins[NUM_OF_ROWS] =
{
	PINS_ROWS
};

static const uint8_t col_pins[NUM_OF_COLS] =
{
	PINS_COLS
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

static const struct entry kbd_entries[][NUM_OF_COLS] =
{
	{ { KEY_JOY_CENTER },  { 'W', '1' },       { 'G', '/' },       { 'S', '4' },       { 'L', '"'  },  { 'H' , ':' } },
	{ { },                 { 'Q', '#' },       { 'R', '3' },       { 'E', '2' },       { 'O', '+'  },  { 'U', '_'  } },
	{ { KEY_BTN_LEFT1 },   { '~', '0' },       { 'F', '6' },       { .mod = MOD_SHL }, { 'K', '\''  }, { 'J', ';'  } },
	{ { },                 { ' ', '\t' },      { 'C', '9' },       { 'Z', '7' },       { 'M', '.'  },  { 'N', ','  } },
	{ { KEY_BTN_LEFT2 },   { .mod = MOD_SYM }, { 'T', '(' },       { 'D', '5' },       { 'I', '-'  },  { 'Y', ')'  } },
	{ { KEY_BTN_RIGHT1 },  { .mod = MOD_ALT }, { 'V', '?' },       { 'X', '8' },       { '$', '`'  },  { 'B', '!'  } },
	{ { },                 { 'A', '*' },       { .mod = MOD_SHR }, { 'P', '@' },       { '\b' },       { '\n', '|' } },
};

#if NUM_OF_BTNS > 0
static const struct entry btn_entries[NUM_OF_BTNS] =
{
	BTN_KEYS
};

static const uint8_t btn_pins[NUM_OF_BTNS] =
{
	PINS_BTNS
};
#endif

#pragma GCC diagnostic pop

static struct
{
	struct key_lock_callback *lock_callbacks;
	struct key_callback *key_callbacks;

	struct list_item list[LIST_SIZE];

	uint32_t last_process_time;

	bool mods[MOD_LAST];

	bool capslock_changed;
	bool capslock;

	bool numlock_changed;
	bool numlock;
} self;

static void transition_to(struct list_item * const p_item, const enum key_state next_state)
{
	const struct entry * const p_entry = p_item->p_entry;

	p_item->state = next_state;

	if (!self.key_callbacks || !p_entry)
		return;

	char chr = p_entry->chr;

	switch (p_entry->mod) {
		case MOD_ALT:
			if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
				chr = KEY_MOD_ALT;
			break;

		case MOD_SHL:
			if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
				chr = KEY_MOD_SHL;
			break;

		case MOD_SHR:
			if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
				chr = KEY_MOD_SHR;
			break;

		case MOD_SYM:
			if (reg_is_bit_set(REG_ID_CFG, CFG_REPORT_MODS))
				chr = KEY_MOD_SYM;
			break;

		default:
		{
			if (reg_is_bit_set(REG_ID_CFG, CFG_USE_MODS)) {
				const bool shift = (self.mods[MOD_SHL] || self.mods[MOD_SHR]) | self.capslock;
				const bool alt = self.mods[MOD_ALT] | self.numlock;

				if (alt) {
					chr = p_entry->symb;
				} else if (!shift && (chr >= 'A' && chr <= 'Z')) {
					chr = (chr + ' ');
				}
			}

			break;
		}
	}

	if (chr != 0) {
		const struct fifo_item item = { chr, next_state };
		if (!fifo_enqueue(item)) {
			if (reg_is_bit_set(REG_ID_CFG, CFG_OVERFLOW_INT)) {
				reg_set_bit(REG_ID_INT, INT_OVERFLOW);
			}

			if (reg_is_bit_set(REG_ID_CFG, CFG_OVERFLOW_ON))
				fifo_enqueue_force(item);
		}

		struct key_callback *cb = self.key_callbacks;
		while (cb) {
			cb->func(chr, next_state);
			cb = cb->next;
		}
	}
}

static void next_item_state(struct list_item * const p_item, const bool pressed)
{
	switch (p_item->state) {
		case KEY_STATE_IDLE:
			if (pressed) {
				if (p_item->p_entry->mod != MOD_NONE)
					self.mods[p_item->p_entry->mod] = true;

				if (!self.capslock_changed && self.mods[MOD_SHR] && self.mods[MOD_ALT]) {
					self.capslock = true;
					self.capslock_changed = true;
				}

				if (!self.numlock_changed && self.mods[MOD_SHL] && self.mods[MOD_ALT]) {
					self.numlock = true;
					self.numlock_changed = true;
				}

				if (!self.capslock_changed && (self.mods[MOD_SHL] || self.mods[MOD_SHR])) {
					self.capslock = false;
					self.capslock_changed = true;
				}

				if (!self.numlock_changed && (self.mods[MOD_SHL] || self.mods[MOD_SHR])) {
					self.numlock = false;
					self.numlock_changed = true;
				}

				if (!self.mods[MOD_ALT]) {
					self.capslock_changed = false;
					self.numlock_changed = false;
				}

				if (self.lock_callbacks && (self.capslock_changed || self.numlock_changed)) {
					struct key_lock_callback *cb = self.lock_callbacks;
					while (cb) {
						cb->func(self.capslock_changed, self.numlock_changed);

						cb = cb->next;
					}
				}

				transition_to(p_item, KEY_STATE_PRESSED);

				p_item->hold_start_time = to_ms_since_boot(get_absolute_time());
			}
			break;

		case KEY_STATE_PRESSED:
			if ((to_ms_since_boot(get_absolute_time()) - p_item->hold_start_time) > (reg_get_value(REG_ID_HLD) * 10)) {
				transition_to(p_item, KEY_STATE_HOLD);
			 } else if(!pressed) {
				transition_to(p_item, KEY_STATE_RELEASED);
			}
			break;

		case KEY_STATE_HOLD:
			if (!pressed)
				transition_to(p_item, KEY_STATE_RELEASED);
			break;
		case KEY_STATE_RELEASED:
		{
			if (p_item->p_entry->mod != MOD_NONE)
				self.mods[p_item->p_entry->mod] = false;

			p_item->p_entry = NULL;
			transition_to(p_item, KEY_STATE_IDLE);
			break;
		}
	}
}

void keyboard_task(void)
{
	if ((to_ms_since_boot(get_absolute_time()) - self.last_process_time) <= reg_get_value(REG_ID_FRQ))
		return;

	for (uint32_t c = 0; c < NUM_OF_COLS; ++c) {
		gpio_pull_up(col_pins[c]);
		gpio_put(col_pins[c], 0);
		gpio_set_dir(col_pins[c], GPIO_OUT);

		for (uint32_t r = 0; r < NUM_OF_ROWS; ++r) {
			const bool pressed = (gpio_get(row_pins[r]) == 0);
			const int32_t key_idx = (int32_t)((r * NUM_OF_COLS) + c);

			int32_t list_idx = -1;
			for (int32_t i = 0; i < LIST_SIZE; ++i) {
				if (self.list[i].p_entry != &((const struct entry*)kbd_entries)[key_idx])
					continue;

				list_idx = i;
				break;
			}

			if (list_idx > -1) {
				next_item_state(&self.list[list_idx], pressed);
				continue;
			}

			if (!pressed)
				continue;

			for (uint32_t i = 0 ; i < LIST_SIZE; ++i) {
				if (self.list[i].p_entry != NULL)
					continue;

				self.list[i].p_entry = &((const struct entry*)kbd_entries)[key_idx];
				self.list[i].state = KEY_STATE_IDLE;
				next_item_state(&self.list[i], pressed);

				break;
			}
		}

		gpio_put(col_pins[c], 1);
		gpio_disable_pulls(col_pins[c]);
		gpio_set_dir(col_pins[c], GPIO_IN);
	}

#if NUM_OF_BTNS > 0
	for (uint32_t b = 0; b < NUM_OF_BTNS; ++b) {
		const bool pressed = (gpio_get(btn_pins[b]) == 0);

		int32_t list_idx = -1;
		for (int32_t i = 0; i < LIST_SIZE; ++i) {
			if (self.list[i].p_entry != &((const struct entry*)btn_entries)[b])
				continue;

			list_idx = i;
			break;
		}

		if (list_idx > -1) {
			next_item_state(&self.list[list_idx], pressed);
			continue;
		}

		if (!pressed)
			continue;

		for (uint32_t i = 0 ; i < LIST_SIZE; ++i) {
			if (self.list[i].p_entry != NULL)
				continue;

			self.list[i].p_entry = &((const struct entry*)btn_entries)[b];
			self.list[i].state = KEY_STATE_IDLE;
			next_item_state(&self.list[i], pressed);

			break;
		}
	}
#endif

	self.last_process_time = to_ms_since_boot(get_absolute_time());
}

void keyboard_add_key_callback(struct key_callback *callback)
{
	// first callback
	if (!self.key_callbacks) {
		self.key_callbacks = callback;
		return;
	}

	// find last and insert after
	struct key_callback *cb = self.key_callbacks;
	while (cb->next)
		cb = cb->next;

	cb->next = callback;
}

void keyboard_add_lock_callback(struct key_lock_callback *callback)
{
	// first callback
	if (!self.lock_callbacks) {
		self.lock_callbacks = callback;
		return;
	}

	// find last and insert after
	struct key_lock_callback *cb = self.lock_callbacks;
	while (cb->next)
		cb = cb->next;

	cb->next = callback;
}

bool keyboard_get_capslock(void)
{
	return self.capslock;
}

bool keyboard_get_numlock(void)
{
	return self.numlock;
}

void keyboard_init(void)
{
	for (int i = 0; i < MOD_LAST; ++i)
		self.mods[i] = false;

	// Rows
	for (uint32_t i = 0; i < NUM_OF_ROWS; ++i) {
		gpio_init(row_pins[i]);
		gpio_pull_up(row_pins[i]);
		gpio_set_dir(row_pins[i], GPIO_IN);
	}

	// Cols
	for(uint32_t i = 0; i < NUM_OF_COLS; ++i) {
		gpio_init(col_pins[i]);
		gpio_set_dir(col_pins[i], GPIO_IN);
	}

	// Btns
#if NUM_OF_BTNS > 0
	for(uint32_t i = 0; i < NUM_OF_BTNS; ++i) {
		gpio_init(btn_pins[i]);
		gpio_set_dir(btn_pins[i], GPIO_IN);
		gpio_pull_up(btn_pins[i]);
	}
#endif
}
