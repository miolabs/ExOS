#ifndef OHCI_DRIVER_H
#define OHCI_DRIVER_H

#include <usb/host.h>
#include <support/usb/ohci/pipes.h>

// prototypes
void ohci_driver_initialize();
void ohci_driver_enumerate(unsigned port, usb_host_device_speed_t speed);

#endif // OHCI_DRIVER_H
