#include "usb_host.h"
#include "usb_otg.h"
#include <support/usb/ohci/driver.h>
#include <support/usb/host_hal.h>
#include <kernel/verbose.h>
#include <kernel/panic.h>

OHCI_OP_REGISTERS *__hc = (OHCI_OP_REGISTERS *)LPC_USB_BASE;

bool hal_usb_host_initialize()
{
	usb_otg_initialize_host();
	unsigned clk = cpu_usbclk();
#ifdef DEBUG
	verbose(VERBOSE_COMMENT, "usb-host", "peripheral clock is %d", clk);
#endif
	ASSERT(clk == 48000000, KERNEL_ERROR_KERNEL_PANIC);
	ohci_driver_initialize();
}