#ifndef OHCI_HUB_H
#define OHCI_HUB_H

#include <usb/host.h>
#include <support/usb/ohci/ohci.h>

void ohci_hub_initialize(usb_host_controller_t *hc);
void ohci_hub_signal();

#endif // OHCI_HUB_H
