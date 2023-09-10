#include "usb_otg_hs.h"
#include "cpu.h"
#include <kernel/verbose.h>
#include <kernel/panic.h>
#include <string.h>

static usb_otg_crs_global_t * const otg_global = (usb_otg_crs_global_t *)(USB_OTG_HS_BASE + 0x000);
static usb_otg_crs_power_t * const otg_power = (usb_otg_crs_power_t *)(USB_OTG_HS_BASE + 0xe00);

static event_t _otg_event;
event_t *__otg_hs_event = &_otg_event;


void usb_otg_hs_initialize()
{
	static bool init_done = false;
	if (!init_done)
	{
		exos_event_create(&_otg_event, EXOS_EVENTF_AUTORESET);
		init_done = true;
	}

	// core initialization
	RCC->AHB1RSTR |= RCC_AHB1RSTR_OTGHRST;
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSEN;	// enable peripheral clock
#ifdef STM32_USB_HS_ULPI
	RCC->AHB1ENR |= RCC_AHB1ENR_OTGHSULPIEN;	// enable ulpi clock
#endif
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_OTGHRST;

unsigned gusbcfg = otg_global->GUSBCFG & (USB_OTG_GUSBCFG_TOCAL | USB_OTG_GUSBCFG_TRDT | (1<<4));
#ifdef STM32_USB_HS_ULPI
	#ifdef USB_OTG_GUSBCFG_ULPI_UTMI_SEL
	// NOTE: select external ULPY HS PHY for devices with internal HS PHY
	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_ULPI_UTMI_SEL;
	#endif
	otg_global->GUSBCFG = gusbcfg /*| USB_OTG_GUSBCFG_ULPIEVBUSI*/ | USB_OTG_GUSBCFG_ULPIIPD | USB_OTG_GUSBCFG_ULPIAR 
		 | USB_OTG_GUSBCFG_ULPIEVBUSD;
#else
	// NOTE: selects internal FS PHY
	otg_global->GUSBCFG = gusbcfg | USB_OTG_GUSBCFG_PHYSEL;
#endif

	while(!(otg_global->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL));
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	otg_global->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;	// soft-reset
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_CSRST);	// NOTE: self-clearing
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	// enable PHY
#ifdef USB_OTG_GCCFG_NOVBUSSENS 
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;
#else
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN;
#endif
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

	otg_global->GAHBCFG |= USB_OTG_GAHBCFG_GINT;
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
	unsigned sta = otg_global->GINTSTS;

	if (sta & USB_OTG_GINTSTS_MMIS)
	{
		otg_global->GINTSTS = USB_OTG_GINTSTS_MMIS; // clear int
	}

	if (sta & USB_OTG_GINTSTS_CMOD)
	{
		__usb_otg_hs_host_irq_handler();
	}
	else
	{
		__usb_otg_hs_device_irq_handler();
	}
}

static usb_host_role_state_t _state;

usb_host_role_state_t usb_otg_hs_role_state()
{
	return _state;
}

void usb_otg_hs_notify(usb_host_role_state_t state)
{
	_state = state;
	exos_event_set(&_otg_event);
}


