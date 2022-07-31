#include "board/board.h"
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>
#include <support/lpc17/gpio.h>

void __board_init()
{
	dma_initialize();

#if defined BOARD_MIOBOARD

	#define LED0_PIN P3_23
	hal_gpio_pin_config(LED0_PIN, GPIOF_OUTPUT);
	// TODO

#elif defined BOARD_LPC1766STK

	pincon_setfunc(0, 21, 0, PINMODE_PULLUP);	// SD_PWR
	hal_gpio_config(0, 1<<21, 1<<21);
	hal_gpio_pin_set(0, 21, 1);
	pincon_setfunc(0, 6, 0, PINMODE_PULLUP);	// SD_CS

#define SDCARD_POWER_PORT 0	
#define SDCARD_POWER_PIN 21	
#define SDCARD_SEL_PORT 0
#define SDCARD_SEL_PIN 6	

	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR

	pincon_setfunc(1, 31, 3, PINMODE_FLOAT);	// AN5

	LPC_GPIO1->FIODIR |= (1<<25);	// LED1
	LPC_GPIO0->FIODIR |= (1<<4);	// LED2
	hal_led_set(0, 0);
	hal_led_set(1, 0);

#elif defined BOARD_LANDTIGER

	pincon_setfunc(3, 26, 0, PINMODE_PULLUP);	// SD_PWR
	hal_gpio_config(3, 1<<26, 1<<26);
	hal_gpio_pin_set(3, 25, 1);
	pincon_setfunc(3, 25, 0, PINMODE_PULLUP);	// SD_CS

#define SDCARD_POWER_PORT 3	
#define SDCARD_POWER_PIN 26	
#define SDCARD_SEL_PORT 3
#define SDCARD_SEL_PIN 25	

	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// USB_UP_LED
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// _USB_PPWR
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// USB_PWRD
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// _USB_OVRCR

	pincon_setfunc(0, 0, 1, PINMODE_PULLUP);	// RD1
	pincon_setfunc(0, 1, 1, PINMODE_PULLUP);	// RT1
	pincon_setfunc(0, 4, 2, PINMODE_PULLUP);	// RD2
	pincon_setfunc(0, 5, 2, PINMODE_PULLUP);	// RT2

	pincon_setfunc(1, 31, 3, PINMODE_FLOAT);	// AN5

	LPC_GPIO2->FIODIR |= 0xFF;	// all 8 lower bits are outputs

#elif defined BOARD_NANO10

	pincon_setfunc(0, 21, 0, PINMODE_PULLUP);	// SD_PWR
	hal_gpio_config(0, 1<<21, 1<<21);
	hal_gpio_pin_set(0, 21, 1);
	pincon_setfunc(1, 21, 0, PINMODE_PULLUP);	// SD_CS
	hal_gpio_config(1, 1<<21, 1<<21);
	hal_gpio_pin_set(1, 21, 1);

#define SDCARD_POWER_PORT 0	
#define SDCARD_POWER_PIN 21	
#define SDCARD_SEL_PORT 1
#define SDCARD_SEL_PIN 21	

	pincon_setfunc(0, 23, 1, PINMODE_FLOAT);	// AN0
	pincon_setfunc(0, 24, 1, PINMODE_FLOAT);	// AN1
	pincon_setfunc(0, 25, 1, PINMODE_FLOAT);	// AN2
	pincon_setfunc(0, 26, 1, PINMODE_FLOAT);	// AN3

	pincon_setfunc(0, 27, 1, PINMODE_PULLUP);	// SDA0
	pincon_setfunc(0, 28, 1, PINMODE_PULLUP);	// SCL0

	pincon_setfunc(0, 29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(0, 30, 1, PINMODE_FLOAT);	// D-
   	pincon_setfunc(2, 9, 1, PINMODE_PULLUP);	// USB_CONNECT

	pincon_setfunc(0, 4, 2, PINMODE_PULLUP);	// RD2
	pincon_setfunc(0, 5, 2, PINMODE_PULLUP);	// RT2

	pincon_setfunc(1, 20, 3, PINMODE_PULLUP);	// SCK0
	pincon_setfunc(1, 21, 3, PINMODE_PULLUP);	// SSEL0
	pincon_setfunc(1, 23, 3, PINMODE_PULLUP);	// MISO0
	pincon_setfunc(1, 24, 3, PINMODE_PULLUP);	// MOSI0

	pincon_setfunc(0, 2, 1, PINMODE_PULLUP);	// TXD0
	pincon_setfunc(0, 3, 1, PINMODE_PULLUP);	// RXD0
	pincon_setfunc(0, 15, 1, PINMODE_PULLUP);	// TXD1
	pincon_setfunc(0, 16, 1, PINMODE_PULLUP);	// RXD1

	pincon_setfunc(2, 0, 1, PINMODE_PULLUP);	// PWM1.1
	pincon_setfunc(2, 1, 1, PINMODE_PULLUP);	// PWM1.2
	pincon_setfunc(2, 2, 1, PINMODE_PULLUP);	// PWM1.3
	pincon_setfunc(2, 3, 1, PINMODE_PULLUP);	// PWM1.4
	pincon_setfunc(2, 4, 1, PINMODE_PULLUP);	// PWM1.5
	pincon_setfunc(2, 5, 1, PINMODE_PULLUP);	// PWM1.6

	LPC_GPIO2->FIODIR |= (1<<6);	// STATUS_LED
	LPC_GPIO1->FIODIR |= (1<<18);	// USB_LED
	LPC_GPIO3->FIODIR |= (1<<25);	// GPS_LED

#else
#error Unsupported Board
#endif
}


void board_led_set(board_led_t led, bool state)
{
	switch(led)
	{
#ifdef LED0_PIN
		case LED_STATUS:	hal_gpio_pin_set(LED0_PIN, !state);	break;
#endif
	}
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
	pincon_setfunc(1, 26, 0, PINMODE_PULLUP);	// CS
//	pincon_setfunc(4, 28, 0, PINMODE_PULLUP);	// A0
	pincon_setfunc(0, 8, 0, PINMODE_PULLUP);	// A0
	pincon_setfunc(4, 29, 0, PINMODE_PULLUP);	// RESET

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



#if defined(SDCARD_POWER_PORT) && defined(SDCARD_POWER_PIN)

void sd_spi_power_control(int power)
{
	hal_gpio_pin_set(SDCARD_POWER_PORT, SDCARD_POWER_PIN, !power);	
	hal_gpio_config(SDCARD_SEL_PORT, 1<<SDCARD_SEL_PIN, power ? 1<<SDCARD_SEL_PIN : 0);

#ifdef BOARD_LPC1766STK
		pincon_setfunc(0, 6, 0, PINMODE_PULLUP);	// SSEL1
		pincon_setfunc(0, 7, power ? 2 : 0, PINMODE_FLOAT);		// SCK1
		pincon_setfunc(0, 8, power ? 2 : 0, PINMODE_PULLUP);	// MISO1
		pincon_setfunc(0, 9, power ? 2 : 0, PINMODE_FLOAT);		// MOSI1
#endif
#if defined BOARD_NANOAHRS || defined BOARD_LANDTIGER
		pincon_setfunc(1, 20, power ? 3 : 0, PINMODE_FLOAT);	// SCK0
		pincon_setfunc(1, 21, 0, PINMODE_PULLUP);	// SSEL0
		pincon_setfunc(1, 23, power ? 3 : 0, PINMODE_PULLUP);	// MISO0
		pincon_setfunc(1, 24, power ? 3 : 0, PINMODE_FLOAT);	// MOSI0
#endif
}

void sd_spi_cs_assert()
{
	hal_gpio_pin_set(SDCARD_SEL_PORT, SDCARD_SEL_PIN, 0);
}

void sd_spi_cs_release()
{
	hal_gpio_pin_set(SDCARD_SEL_PORT, SDCARD_SEL_PIN, 1);
}

#endif

