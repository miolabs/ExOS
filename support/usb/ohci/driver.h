#ifndef OHCI_DRIVER_H
#define OHCI_DRIVER_H

#include <usb/host.h>
#include <support/usb/ohci/pipes.h>

// prototypes
void ohci_driver_initialize();

USB_HOST_DEVICE *ohci_device_create(int port, USB_HOST_DEVICE_SPEED speed);

#endif // OHCI_DRIVER_H
