#ifndef STM32F4_USB_FS_HOST_H
#define STM32F4_USB_FS_HOST_H

#include <usb/host.h>
#include <kernel/dispatch.h>


void usb_otg_fs_init_as_host(dispatcher_context_t *context);

#endif // STM32F4_USB_FS_HOST_H

