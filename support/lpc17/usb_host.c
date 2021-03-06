#include "usb_host.h"
#include "usb_otg.h"
#include <support/usb/ohci/driver.h>
#include <support/usb/host_hal.h>

OHCI_OP_REGISTERS *__hc = (OHCI_OP_REGISTERS *)LPC_USB_BASE;

int hal_usb_host_initialize()
{
	usb_otg_initialize_host();

	ohci_driver_initialize();
}