#ifndef OHCI_BUFFERS_H
#define OHCI_BUFFERS_H

#include <support/usb/ohci/driver.h>

// prototypes
void ohci_buffers_initialize();
OHCI_STD *ohci_buffers_alloc_std();
void ohci_buffers_release_std(OHCI_STD *td);
OHCI_SED *ohci_buffers_alloc_sed();
void ohci_buffers_release_sed(OHCI_SED *ed);

#endif // OHCI_BUFFERS_H
