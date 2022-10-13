#include "usb_otg_hs.h"
#include "cpu.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <string.h>

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_HS_BASE + 0x000);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *)(USB_OTG_HS_BASE + 0xe00);

#define STM32_USB_HS_ULPI	// FIXME: temporary

void usb_otg_hs_initialize()
{
	// core initialization
	RCC->AHB1RSTR |= RCC_AHB1RSTR_OTGHRST;
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSEN;	// enable peripheral clock
#ifdef STM32_USB_HS_ULPI
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSULPIEN;	// enable ulpi clock
#endif
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_OTGHRST;

#ifdef STM32_USB_HS_ULPI
	otg_global->GUSBCFG |= (1<<4); // FIXME: undefined USB_OTG_GUSBCFG_ULPISEL
	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_ULPIEVBUSD | USB_OTG_GUSBCFG_ULPIIPD;
#else
	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;
#endif

	while(!(otg_global->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL));
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks


	otg_global->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;	// soft-reset
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_CSRST);	// NOTE: self-clearing
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	// enable PHY
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN/* | USB_OTG_GCCFG_NOVBUSSENS*/;	// <--- FIXME
	exos_thread_sleep(20);

#if defined(STM32_USB_HS_ULPI) && defined(DEBUG)
	unsigned char ulpi_id[4];
	for(unsigned i = 0; i < sizeof(ulpi_id); i++) 
	{
		if (!usb_otg_hs_ulpi_read(i, &ulpi_id[i]))
		{
			verbose(VERBOSE_ERROR, "otg-hs", "ulpi-phy read failed!");
			break;
		}
	}
	unsigned short vendor_id = (ulpi_id[1] << 8) | ulpi_id[0];
	const char *vendor = vendor_id == 0x0424 ? "Microchip" : "???";
	verbose(VERBOSE_DEBUG, "otg-hs", "ulpi phy vendor=%04x (%s) id=%02x%02x", 
		vendor_id, vendor, ulpi_id[3], ulpi_id[2]);
#endif

	otg_global->GINTMSK = USB_OTG_GINTMSK_MMISM;	// Mode MISmatch Mask

	otg_global->GAHBCFG |= OTG_HS_GAHBCFG_GINTMASK;
	NVIC_EnableIRQ(OTG_HS_IRQn);
}

bool usb_otg_hs_ulpi_read(unsigned short addr, unsigned char *pdata)
{
	bool done = false;
	otg_global->PHYCR = OTG_HS_PHYCR_NEW | (addr << 16);
	for (unsigned timeout = 100; timeout != 0; timeout--)
	{
		unsigned val = otg_global->PHYCR;
		if (val & OTG_HS_PHYCR_DONE)
		{
			*pdata = val & OTG_HS_PHYCR_DATA;
			done = true;
			break;
		}
	}
	return done;
}

bool usb_otg_hs_ulpi_write(unsigned short addr, unsigned char data)
{
	bool done = false;
	otg_global->PHYCR = OTG_HS_PHYCR_NEW | OTG_HS_PHYCR_RW | (addr << 16) | data;
	for (unsigned timeout = 10; timeout != 0; timeout--)
	{
		unsigned val = otg_global->PHYCR;
		if (val & OTG_HS_PHYCR_DONE)
		{
			done = true;
			break;
		}
	}
	return done;
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


