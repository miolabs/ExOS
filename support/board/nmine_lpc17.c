#include <support/board_hal.h>
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>


void hal_board_initialize()
{
	dma_initialize();

#if defined BOARD_LPC1766STK

	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED
	pincon_setfunc(1, 19, 2);	// _USB_PPWR
	pincon_setfunc(1, 22, 2);	// USB_PWRD
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR

	pincon_setfunc(1, 31, 3);	// AN5

	LPC_GPIO1->FIODIR |= (1<<25);	// LED1
	LPC_GPIO0->FIODIR |= (1<<4);	// LED2
	hal_led_set(0, 0);
	hal_led_set(1, 0);

#elif defined BOARD_LANDTIGER

	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED
	pincon_setfunc(1, 19, 2);	// _USB_PPWR
	pincon_setfunc(1, 22, 2);	// USB_PWRD
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR

	pincon_setfunc(0, 0, 1);	// RD1
	pincon_setfunc(0, 1, 1);	// RT1
	pincon_setfunc(0, 4, 2);	// RD2
	pincon_setfunc(0, 5, 2);	// RT2

	pincon_setfunc(1, 31, 3);	// AN5

	LPC_GPIO2->FIODIR |= 0xFF;	// all 8 lower bits are outputs

#elif defined BOARD_NANO10

	pincon_setfunc(0, 23, 1);	// AN0
	pincon_setfunc(0, 24, 1);	// AN1
	pincon_setfunc(0, 25, 1);	// AN2
	pincon_setfunc(0, 26, 1);	// AN3

	pincon_setfunc(0, 27, 1);	// SDA0
	pincon_setfunc(0, 28, 1);	// SCL0

	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
   	pincon_setfunc(2, 9, 1);	// USB_CONNECT

	pincon_setfunc(0, 4, 2);	// RD2
	pincon_setfunc(0, 5, 2);	// RT2

	pincon_setfunc(1, 20, 3);	// SCK0
	pincon_setfunc(1, 21, 3);	// SSEL0
	pincon_setfunc(1, 23, 3);	// MISO0
	pincon_setfunc(1, 24, 3);	// MOSI0

	pincon_setfunc(0, 2, 1);	// TXD0
	pincon_setfunc(0, 3, 1);	// RXD0
	pincon_setfunc(0, 15, 1);	// TXD1
	pincon_setfunc(0, 16, 1);	// RXD1

	pincon_setfunc(2, 0, 1);	// PWM1.1
	pincon_setfunc(2, 1, 1);	// PWM1.2
	pincon_setfunc(2, 2, 1);	// PWM1.3
	pincon_setfunc(2, 3, 1);	// PWM1.4
	pincon_setfunc(2, 4, 1);	// PWM1.5
	pincon_setfunc(2, 5, 1);	// PWM1.6

	LPC_GPIO2->FIODIR |= (1<<6);	// STATUS_LED
	LPC_GPIO1->FIODIR |= (1<<18);	// USB_LED
	LPC_GPIO3->FIODIR |= (1<<25);	// GPS_LED

#else
#error Unsupported Board
#endif
}


#if defined BOARD_LPC1766STK
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
			if (state) 
				LPC_GPIO1->FIOCLR = (1<<25);
			else
				LPC_GPIO1->FIOSET = (1<<25);
			break;
		case 1:
			if (state)
				LPC_GPIO0->FIOCLR = (1<<4);
			else
				LPC_GPIO0->FIOSET = (1<<4);
			break;
	}
}

#include <support/lcd/lcd.h>
#define LCD_CS_PORT LPC_GPIO1	// CS_UEXT = P1.26
#define LCD_CS_MASK (1<<26)

//#define LCD_A0_PORT LPC_GPIO4	// TXD3 = P4.28
//#define LCD_A0_MASK (1<<28)
#define LCD_A0_PORT LPC_GPIO0	// MISO1 = P0.8
#define LCD_A0_MASK (1<<8)

#define LCD_RESET_PORT LPC_GPIO4	// RXD3 = P4.29
#define LCD_RESET_MASK (1<<29)

void lcdcon_gpo_initialize()
{
	PINSEL3bits.P1_26 = 0;
//	PINSEL9bits.P4_28 = 0;
	PINSEL0bits.P0_8 = 0;
	PINSEL9bits.P4_29 = 0;

	LCD_CS_PORT->FIOSET = LCD_CS_MASK;
	LCD_CS_PORT->FIODIR |= LCD_CS_MASK;
	LCD_A0_PORT->FIOSET = LCD_A0_MASK;
	LCD_A0_PORT->FIODIR |= LCD_A0_MASK;
	LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
	LCD_RESET_PORT->FIODIR |= LCD_RESET_MASK;
}

void lcdcon_gpo(LCDCON_GPO gpo)
{
	if (gpo & LCDCON_GPO_CS) LCD_CS_PORT->FIOCLR = LCD_CS_MASK;
	else LCD_CS_PORT->FIOSET = LCD_CS_MASK;
	if (gpo & LCDCON_GPO_A0) LCD_A0_PORT->FIOCLR = LCD_A0_MASK;
	else LCD_A0_PORT->FIOSET = LCD_A0_MASK;
	if (gpo & LCDCON_GPO_RESET) LCD_RESET_PORT->FIOCLR = LCD_RESET_MASK;
	else LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
}

#endif

#if defined(BOARD_LANDTIGER)
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_STATUS:
		case 0:
			if (state) 
				LPC_GPIO2->FIOSET = (1<<0);
			else
				LPC_GPIO2->FIOCLR = (1<<0);
			break;
		case LED_SDCARD:
		case 1:
			if (state)
				LPC_GPIO2->FIOSET = (1<<1);
			else
				LPC_GPIO2->FIOCLR = (1<<1);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if (state)
				LPC_GPIO2->FIOSET = (1<<led);
			else
				LPC_GPIO2->FIOCLR = (1<<led);
			break;
			
	}
}

#endif


#if defined(BOARD_MINILCD)

void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_STATUS:
		case 0:
			if (state)
				LPC_GPIO1->FIOSET = (1<<18); // USB_LED
			else
				LPC_GPIO1->FIOCLR = (1<<18); // USB_LED
			break;
	}
}

#include <support/lcd/lcd.h>
#define LCD_CS_PORT LPC_GPIO0	// LCD_CS = P0.16
#define LCD_CS_MASK (1<<16)
#define LCD_A0_PORT LPC_GPIO2	// LCD_A0 = P2.9
#define LCD_A0_MASK (1<<9)
#define LCD_RESET_PORT LPC_GPIO0	// LCD_RST = P0.22
#define LCD_RESET_MASK (1<<22)
#define LCD_BL_PORT LPC_GPIO2	// LCD_BL = P2.0
#define LCD_BL_MASK (1<<0)

void lcdcon_gpo_initialize()
{
	PINSEL1bits.P0_16 = 0;
	PINSEL4bits.P2_9 = 0;
	PINSEL1bits.P0_22 = 0;
	PINSEL4bits.P2_0 = 0;

	LCD_CS_PORT->FIOSET = LCD_CS_MASK;
	LCD_CS_PORT->FIODIR |= LCD_CS_MASK;
	LCD_A0_PORT->FIOSET = LCD_A0_MASK;
	LCD_A0_PORT->FIODIR |= LCD_A0_MASK;
	LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
	LCD_RESET_PORT->FIODIR |= LCD_RESET_MASK;
	LCD_BL_PORT->FIOCLR = LCD_BL_MASK;
	LCD_BL_PORT->FIODIR |= LCD_BL_MASK;
}

void lcdcon_gpo(LCDCON_GPO gpo)
{
	if (gpo & LCDCON_GPO_CS) LCD_CS_PORT->FIOCLR = LCD_CS_MASK;
	else LCD_CS_PORT->FIOSET = LCD_CS_MASK;
	if (gpo & LCDCON_GPO_A0) LCD_A0_PORT->FIOCLR = LCD_A0_MASK;
	else LCD_A0_PORT->FIOSET = LCD_A0_MASK;
	if (gpo & LCDCON_GPO_RESET) LCD_RESET_PORT->FIOCLR = LCD_RESET_MASK;
	else LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
}

void lcdcon_gpo_backlight(int enable)
{
	if (enable)
	{
		LCD_BL_PORT->FIOSET = LCD_BL_MASK;
	}
	else
	{
		LCD_BL_PORT->FIOCLR = LCD_BL_MASK;
	}
}

#endif

#if defined(BOARD_NANO10)
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_STATUS:
		case 0:
			if (state) LPC_GPIO2->FIOSET = (1<<6);
			else LPC_GPIO2->FIOCLR = (1<<6);
			break;
		case LED_GPS:
		case 1:
			if (state) LPC_GPIO3->FIOSET = (1<<25);
			else LPC_GPIO3->FIOCLR = (1<<25);
			break;
		case LED_SDCARD:
		case 2:
			if (state) LPC_GPIO1->FIOSET = (1<<18);
			else LPC_GPIO1->FIOCLR = (1<<18);
			break;
	}
}

#endif



#if defined(SDCARD_POWER_PORT) && defined(SDCARD_POWER_MASK)

#ifdef SDCARD_POWER_INVERTED
#define SDCARD_POWER_ON SDCARD_POWER_PORT->FIOCLR = SDCARD_POWER_MASK
#define SDCARD_POWER_OFF SDCARD_POWER_PORT->FIOSET = SDCARD_POWER_MASK
#else
#define SDCARD_POWER_ON SDCARD_POWER_PORT->FIOSET = SDCARD_POWER_MASK
#define SDCARD_POWER_OFF SDCARD_POWER_PORT->FIOCLR = SDCARD_POWER_MASK
#endif

void sd_spi_power_control(int power)
{
	if (power)
	{
		SDCARD_POWER_OFF;
		SDCARD_POWER_PORT->FIODIR |= SDCARD_POWER_MASK;
		for(volatile int i = 0; i < 100000; i++);
		SDCARD_POWER_ON;
	}
	else
	{
		SDCARD_POWER_OFF;
		SDCARD_POWER_PORT->FIODIR |= SDCARD_POWER_MASK;
	}
}

#endif

