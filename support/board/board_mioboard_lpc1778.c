#include <support/board_hal.h>
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>

void hal_board_initialize()
{
	dma_initialize();

#if defined BOARD_MIOBOARD

	pincon_setfunc(0, 27, 1, PINMODE_PULLUP);	// SDA0
	pincon_setfunc(0, 28, 1, PINMODE_PULLUP);	// SCL0
	pincon_setfunc(0, 19, 3, PINMODE_PULLUP);	// SDA1
	pincon_setfunc(0, 20, 3, PINMODE_PULLUP);	// SCL1
	pincon_setfunc(0, 10, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SDA2
	pincon_setfunc(0, 11, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SCL2

	// usb1
	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD1
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1, PINMODE_FLOAT);	// D+ 
	pincon_setfunc(0, 12, 1, PINMODE_PULLUP);	// _USB_PPWR2

	pincon_setfunc(0, 0, 1, PINMODE_PULLUP);	// CAN_RD1
	pincon_setfunc(0, 1, 1, PINMODE_PULLUP);	// CAN_TD1 
	pincon_setfunc(2, 7, 1, PINMODE_PULLUP);	// CAN_RD2
	pincon_setfunc(2, 8, 1, PINMODE_PULLUP);	// CAN_TD2 

	#define LED1_PORT LPC_GPIO3
	#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;

#elif defined BOARD_MIOBOARD2

	pincon_setfunc(0, 27, 1, PINMODE_PULLUP);	// SDA0
	pincon_setfunc(0, 28, 1, PINMODE_PULLUP);	// SCL0
	pincon_setfunc(0, 19, 3, PINMODE_PULLUP);	// SDA1
	pincon_setfunc(0, 20, 3, PINMODE_PULLUP);	// SCL1
	pincon_setfunc(0, 10, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SDA2
	pincon_setfunc(0, 11, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SCL2

	// usb1
	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD1
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 13, 1, PINMODE_PULLUP);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1, PINMODE_PULLUP);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1, PINMODE_PULLUP);	// USB_PWRD2
	pincon_setfunc(1, 31, 1, PINMODE_PULLUP);	// _USB_OVRCR2

	//uart1
	pincon_setfunc(2, 0, 2, PINMODE_PULLUP);	// U1_TXD
	pincon_setfunc(2, 1, 2, PINMODE_PULLUP);	// U1_RXD
	pincon_setfunc(2, 2, 2, PINMODE_PULLUP);	// U1_CTS
	pincon_setfunc(2, 7, 2, PINMODE_PULLUP);	// U1_RTS

    LPC_GPIO0->DIR |= (1<<1);
	LPC_GPIO0->CLR = (1<<1);
	LPC_GPIO0->SET = (1<<1);

	pincon_setfunc(0, 0, 1, PINMODE_PULLUP);	// CAN_RD1
	pincon_setfunc(0, 1, 1, PINMODE_PULLUP);	// CAN_TD1 

	#define LED1_PORT LPC_GPIO3
	#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;
	LED1_PORT->DIR |= LED1_MASK;

#elif defined BOARD_MIODOCK

	pincon_setfunc(0, 27, 1, PINMODE_PULLUP);	// SDA0
	pincon_setfunc(0, 28, 1, PINMODE_PULLUP);	// SCL0
	pincon_setfunc(0, 19, 3, PINMODE_PULLUP);	// SDA1
	pincon_setfunc(0, 20, 3, PINMODE_PULLUP);	// SCL1

	pincon_setfunc(0, 10, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SDA2
	pincon_setfunc(0, 11, 2, PINMODE_PULLUP | PINMODEF_HYS | PINMODEF_OPEN_DRAIN);	// SCL2

	// usb1
	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD1
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 13, 1, PINMODE_PULLUP);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1, PINMODE_PULLUP);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1, PINMODE_PULLUP);	// USB_PWRD2
	pincon_setfunc(1, 31, 1, PINMODE_PULLUP);	// _USB_OVRCR2

	//uart1
	pincon_setfunc(0, 15, 1, PINMODE_PULLUP);	// U1_TXD
	pincon_setfunc(0, 16, 1, PINMODE_PULLUP);	// U1_RXD
	pincon_setfunc(0, 17, 1, PINMODE_PULLUP);	// U1_CTS
	pincon_setfunc(2, 7, 2, PINMODE_PULLUP);	// U1_RTS

//	pincon_setfunc(0, 0, 1, PINMODE_PULLUP);	// CAN_RD1
//	pincon_setfunc(0, 1, 1, PINMODE_PULLUP);	// CAN_TD1 
//	pincon_setfunc(2, 7, 1, PINMODE_PULLUP);	// CAN_RD2
//	pincon_setfunc(2, 8, 1, PINMODE_PULLUP);	// CAN_TD2 

	#define LED1_PORT LPC_GPIO3
	#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;
	LED1_PORT->DIR |= LED1_MASK;

#elif defined BOARD_UMFI

	pincon_setfunc(0, 27, 1, PINMODE_PULLUP);	// SDA0
	pincon_setfunc(0, 28, 1, PINMODE_PULLUP);	// SCL0

	// usb1
	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD1
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 13, 1, PINMODE_PULLUP);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1, PINMODE_PULLUP);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1, PINMODE_PULLUP);	// USB_PWRD2
	pincon_setfunc(1, 31, 1, PINMODE_PULLUP);	// _USB_OVRCR2

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




