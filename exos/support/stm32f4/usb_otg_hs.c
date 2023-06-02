#include "usb_otg_hs.h"
#include "cpu.h"
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
	RCC->AHB1RSTR ^= RCC_AHB1RSTR_OTGHRST;

	otg_global->GUSBCFG |= USB_OTG_GUSBCFG_PHYSEL;

	while(!(otg_global->GRSTCTL & USB_OTG_GRSTCTL_AHBIDL));
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	otg_global->GRSTCTL |= USB_OTG_GRSTCTL_CSRST;	// soft-reset
	while(otg_global->GRSTCTL & USB_OTG_GRSTCTL_CSRST);	// NOTE: self-clearing
	for(unsigned volatile i = 0; i < 100; i++);	// wait 3 phy clks

	// enable PHY
#ifdef USB_OTG_GCCFG_NOVBUSSENS
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN | USB_OTG_GCCFG_NOVBUSSENS;	// <--- FIXME
#else
	otg_global->GCCFG = USB_OTG_GCCFG_PWRDWN;
#endif
	exos_thread_sleep(20);

	otg_global->GINTMSK = USB_OTG_GINTMSK_MMISM;	// Mode MISmatch Mask

	otg_global->GAHBCFG |= OTG_FS_GAHBCFG_GINTMASK;
	NVIC_EnableIRQ(OTG_HS_IRQn);
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


