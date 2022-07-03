#pragma once

#include <stdint.h>
#include <stdbool.h>

uint64_t last_triggered;
bool idling;
bool alarm_is_set;
uint8_t backlight_level;

void backlight_trigger(void);
void backlight_sync(void);
void backlight_init(void);
