#ifdef PICO_BUILD

#include "platform.h"

#include <pico/stdlib.h>

#ifdef PICO_RP2040
#include <RP2040.h>
#else
#include <RP2350.h>
#endif

static int64_t schedule_cb(alarm_id_t id, void *user_data)
{
	struct platform_scheduled_callback *callback = (struct platform_scheduled_callback *)user_data;

	return callback->func(callback->ctx) * 1000;
}

uint32_t platform_millis(void)
{
	return to_ms_since_boot(get_absolute_time());
}

void platform_schedule_callback(uint32_t ms, struct platform_scheduled_callback *callback, void *ctx)
{
	add_alarm_in_ms(ms, schedule_cb, (void*)callback, true);
}

void platform_sleep_ms(uint32_t ms)
{
	busy_wait_ms(ms);
}

void platform_reset(void)
{
	NVIC_SystemReset();
}

#endif
