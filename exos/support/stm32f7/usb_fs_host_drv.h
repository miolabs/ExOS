#ifndef STM32F7_USB_FS_HOST_H
#define STM32F7_USB_FS_HOST_H

#include <usb/host.h>
#include <kernel/dispatch.h>


usb_host_controller_t *usb_fs_init_as_host(dispatcher_context_t *context);

#endif // STM32F7_USB_FS_HOST_H

