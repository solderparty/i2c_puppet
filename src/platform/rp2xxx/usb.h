#pragma once

typedef struct mutex mutex_t;

mutex_t *platform_usb_get_mutex(void);
