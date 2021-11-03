#pragma once

typedef struct mutex mutex_t;

mutex_t *usb_get_mutex(void);

void usb_init(void);
