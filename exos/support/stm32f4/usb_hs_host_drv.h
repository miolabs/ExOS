#ifndef STM32F4_USB_OTG_HS_HOST_H
#define STM32F4_USB_OTG_HS_HOST_H

#include <usb/host.h>
#include <kernel/dispatch.h>


usb_host_controller_t *usb_hs_init_as_host(dispatcher_context_t *context);

#endif // STM32F4_USB_OTG_HS_HOST_H

