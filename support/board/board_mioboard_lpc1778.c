#include <support/board_hal.h>
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>

void hal_board_initialize()
{
	dma_initialize();

#if defined BOARD_MIOBOARD

	pincon_setfunc(0, 27, 1);	// SDA0
	pincon_setfunc(0, 28, 1);	// SCL0
	pincon_setfunc(0, 19, 3);	// SDA1
	pincon_setfunc(0, 20, 3);	// SCL1
	pincon_setfunc(0, 10, 2);	// SDA2
	pincon_setfunc(0, 11, 2);	// SCL2

	// usb1
	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2);	// USB_PWRD1
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1);	// D+ 
	pincon_setfunc(0, 12, 1);	// _USB_PPWR2

	pincon_setfunc(0, 0, 1);	// CAN_RD1
	pincon_setfunc(0, 1, 1);	// CAN_TD1 
	pincon_setfunc(2, 7, 1);	// CAN_RD2
	pincon_setfunc(2, 8, 1);	// CAN_TD2 

	#define LED1_PORT LPC_GPIO3
	#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;

#elif defined BOARD_MIOBOARD2

	pincon_setfunc(0, 27, 1);	// SDA0
	pincon_setfunc(0, 28, 1);	// SCL0
	pincon_setfunc(0, 19, 3);	// SDA1
	pincon_setfunc(0, 20, 3);	// SCL1
	pincon_setfunc(0, 10, 2);	// SDA2
	pincon_setfunc(0, 11, 2);	// SCL2
	pincon_setmode(0, 10, IOCON_MODE_PULL_UP, PINCONF_HYS | PINCONF_OPEN_DRAIN);
	pincon_setmode(0, 11, IOCON_MODE_PULL_UP, PINCONF_HYS | PINCONF_OPEN_DRAIN);

	// usb1
	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2);	// USB_PWRD1
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1);	// D+
	pincon_setfunc(0, 13, 1);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1);	// USB_PWRD2
	pincon_setfunc(1, 31, 1);	// _USB_OVRCR2
	
//	pincon_setfunc(0, 0, 1);	// CAN_RD1
//	pincon_setfunc(0, 1, 1);	// CAN_TD1 
//	pincon_setfunc(2, 7, 1);	// CAN_RD2
//	pincon_setfunc(2, 8, 1);	// CAN_TD2 

	#define LED1_PORT LPC_GPIO3
	#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;

#elif defined BOARD_UMFI

	pincon_setfunc(0, 27, 1);	// SDA0
	pincon_setfunc(0, 28, 1);	// SCL0

	// usb1
	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2);	// USB_PWRD1
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1);	// D+
	pincon_setfunc(0, 13, 1);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1);	// USB_PWRD2
	pincon_setfunc(1, 31, 1);	// _USB_OVRCR2

#else
#error Unsupported Board
#endif
}


void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_STATUS:
		case 0:
#if defined LED1_PORT
			if (state)
				LED1_PORT->DIR |= LED1_MASK;
			else
				LED1_PORT->DIR &= ~LED1_MASK;
#endif
			break;
	}
}




