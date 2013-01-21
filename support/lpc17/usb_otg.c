#include "usb_otg.h"
#include "cpu.h"
#include <support/board_hal.h>
#include <CMSIS/LPC17xx.h>

void ohci_isr() __attribute__((__weak__));
void usb_device_isr() __attribute__((__weak__));

static void _isr();

void usb_otg_initialize_host()
{
	hal_board_init_pinmux(HAL_RESOURCE_USBHOST, 0);

	// enable power control for peripheral
    LPC_SC->PCONP |= PCONP_PUSB; 

	// enable USB Host clock
	LPC_USB->OTGClkCtrl = OTGClkCtrl_HOST_CLK_EN;
    while ((LPC_USB->OTGClkSt & OTGClkSt_HOST_CLK_ON) == 0);

	// configure OTG features
	OTGClkCtrlBits.OTG_CLK = 1;
    while ((OTGClkStBits.OTG_CLK) == 0);
	LPC_USB->OTGStCtrl = 0;
	OTGStCtrlBits.PORT_FUNC = 1;	// both ports are host
	OTGClkCtrlBits.OTG_CLK = 0;		// disable OTG module

	NVIC_EnableIRQ(USB_IRQn);
}

void usb_otg_initialize_device()
{
	hal_board_init_pinmux(HAL_RESOURCE_USBDEV, 0);

	// enable power control for peripheral
    LPC_SC->PCONP |= PCONP_PUSB; 

	// enable AHB and USB Device peripheral clock
	LPC_USB->OTGClkCtrl = OTGClkCtrl_AHB_CLK_EN | OTGClkCtrl_DEV_CLK_EN;
    while ((LPC_USB->OTGClkSt & (OTGClkSt_AHB_CLK_ON | OTGClkSt_DEV_CLK_ON)) != 
		(OTGClkSt_AHB_CLK_ON | OTGClkSt_DEV_CLK_ON));

#if defined USB_DEVICE_PORT2
	// configure OTG features
	OTGClkCtrlBits.OTG_CLK = 1;
    while ((OTGClkStBits.OTG_CLK) == 0);
	LPC_USB->OTGStCtrl = 0;
	OTGStCtrlBits.PORT_FUNC = 3;	// port 1 is host, port 2 is device
	OTGClkCtrlBits.OTG_CLK = 0;		// disable OTG module
#endif

	NVIC_EnableIRQ(USB_IRQn);
}

void usb_otg_device_int_control(int enable)
{
	// NOTE: switch EN_USB_INTS bit (all other bits are read_only)
	if (enable)
	{
		LPC_SC->USBIntSt = USBIntSt_EN_USB_INTS;
	}
	else
	{
		LPC_SC->USBIntSt = 0;
	}
}

void USB_IRQHandler()
{
    if (LPC_SC->USBIntSt & USBIntSt_USB_HOST_INT) 
	{
		ohci_isr();
	}
	else if (LPC_SC->USBIntSt & (USBIntSt_USB_INT_REQ_LP | USBIntSt_USB_INT_REQ_HP | USBIntSt_USB_INT_REQ_DMA))
	{
		usb_device_isr();
	}
}


