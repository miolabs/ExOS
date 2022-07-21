#include "usb_otg_fs.h"
#include "cpu.h"
#include <kernel/panic.h>
#include <string.h>

//#define USBD_IEP_COUNT 4
//#define USBD_OEP_COUNT 4

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_FS_BASE + 0x000);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *)(USB_OTG_FS_BASE + 0xe00);

void usb_otg_initialize()
{
	// core initialization
	RCC->AHB2RSTR |= RCC_AHB2RSTR_OTGFSRST;
	RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;	// enable peripheral clock
	RCC->AHB2RSTR ^= RCC_AHB2RSTR_OTGFSRST;

	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

	while(!(otg_global->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL));
	otg_global->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;	// soft-reset
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_CSRST);	// NOTE: self-clearing
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	// enable PHY
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN;	
	exos_thread_sleep(20);

	otg_global->GINTMSK = USB_OTG_GINTMSK_MMISM;	// Mode MISmatch Mask

	otg_global->GAHBCFG |= OTG_FS_GAHBCFG_GINTMASK;
	NVIC_EnableIRQ(OTG_FS_IRQn);
}

void OTG_FS_IRQHandler()
{
	if (otg_global->GINTSTS & USB_OTG_GINTSTS_MMIS)
	{
		otg_global->GINTSTS = USB_OTG_GINTSTS_MMIS; // clear int
	}

	__usb_otg_host_irq_handler();
    __usb_otg_device_irq_handler();
}


