#ifndef OHCI_HUB_H
#define OHCI_HUB_H

#include <support/usb/ohci/ohci.h>

typedef enum
{
	OHCI_HUB_PORT_CONNECTED = (1 << 0),
	OHCI_HUB_PORT_FULLSPEED = (1 << 1),
} OHCI_HUB_PORT_STATUS;

void ohci_hub_initialize();
void ohci_hub_signal();

#endif // OHCI_HUB_H
