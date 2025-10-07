#include "backlight.h"

#include "platform.h"
#include "reg.h"
#include "variant.h"

void backlight_sync(void)
{
	platform_backlight_set(reg_get_value(REG_ID_BKL));
}

void backlight_init(void)
{
	platform_backlight_init(PIN_BKL);

	backlight_sync();
}
