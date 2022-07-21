#ifndef STM32F1_USB_OTG_HOST_H
#define STM32F1_USB_OTG_HOST_H

#include <usb/host.h>
#include <kernel/dispatch.h>


void usb_otg_init_as_host(dispatcher_context_t *context);

#endif // STM32F1_USB_OTG_HOST_H

