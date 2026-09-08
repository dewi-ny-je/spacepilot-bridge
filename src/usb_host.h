#ifndef USB_HOST_H
#define USB_HOST_H

#include <stdint.h>

/* Call from the main loop after tuh_task(). */
void usb_host_task(void);

/* Time (ms since boot) of the last report received from the source. */
uint32_t usb_host_last_activity_ms(void);

#endif
