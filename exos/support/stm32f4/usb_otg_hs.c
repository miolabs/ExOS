#include "usb_otg_hs.h"
#include "cpu.h"
#include <kernel/panic.h>
#include <string.h>

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_HS_BASE + 0x000);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *)(USB_OTG_HS_BASE + 0xe00);

void usb_otg_hs_initialize()
{
	// core initialization
	RCC->AHB1RSTR |= RCC_AHB1RSTR_OTGHRST;
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSEN;	// enable peripheral clock
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_OTGHRST;

	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

	while(!(otg_global->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL));
	otg_global->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;	// soft-reset
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_CSRST);	// NOTE: self-clearing
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	// enable PHY
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;	// <--- FIXME
	exos_thread_sleep(20);

	otg_global->GINTMSK = USB_OTG_GINTMSK_MMISM;	// Mode MISmatch Mask

	otg_global->GAHBCFG |= OTG_FS_GAHBCFG_GINTMASK;
	NVIC_EnableIRQ(OTG_HS_IRQn);
}

void OTG_HS_IRQHandler()
{
	if (otg_global->GINTSTS & USB_OTG_GINTSTS_MMIS)
	{
		otg_global->GINTSTS = USB_OTG_GINTSTS_MMIS; // clear int
	}

	__usb_otg_hs_host_irq_handler();
    __usb_otg_hs_device_irq_handler();
}


