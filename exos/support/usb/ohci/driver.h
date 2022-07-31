#ifndef OHCI_DRIVER_H
#define OHCI_DRIVER_H

#include <usb/host.h>
#include <support/usb/ohci/pipes.h>

// prototypes
void ohci_driver_initialize();
void ohci_driver_enumerate(int port, USB_HOST_DEVICE_SPEED speed);

int ohci_device_create(USB_HOST_DEVICE *device, int port, USB_HOST_DEVICE_SPEED speed);
void ohci_device_destroy(USB_HOST_DEVICE *device);

#endif // OHCI_DRIVER_H
