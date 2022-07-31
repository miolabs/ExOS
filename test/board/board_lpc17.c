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

	#define SDCARD_POWER_PIN P0_21
	hal_gpio_pin_config(SDCARD_POWER_PIN, GPIOF_OUTPUT);
	hal_gpio_pin_set(SDCARD_POWER_PIN, 1);
	#define SDCARD_SEL_PIN P0_6	
	hal_gpio_pin_config(SDCARD_SEL_PIN, GPIOF_OUTPUT | GPIOF_PULLUP);
	hal_gpio_pin_set(SDCARD_SEL_PIN, 1);

	pincon_setfunc(P0_29, 1, PINMODE_FLOAT);	// D+
	pincon_setfunc(P0_30, 1, PINMODE_FLOAT);	// D-
	pincon_setfunc(P1_18, 1, PINMODE_PULLUP);	// USB_UP_LED
	pincon_setfunc(P1_19, 2, PINMODE_PULLUP);	// _USB_PPWR
	pincon_setfunc(P1_22, 2, PINMODE_PULLUP);	// USB_PWRD
	pincon_setfunc(P1_27, 2, PINMODE_PULLUP);	// _USB_OVRCR

	pincon_setfunc(P1_31, 3, PINMODE_FLOAT);	// AN5

	#define LED0_PIN P1_25	// LED1 silk
	hal_gpio_pin_config(LED0_PIN, GPIOF_OUTPUT);
	#define LED1_PIN P0_4	// LED2 silk
	hal_gpio_pin_config(LED1_PIN, GPIOF_OUTPUT);

#else
#error Unsupported Board
#endif

	board_led_set(0, 0);
	board_led_set(1, 0);
}


void board_led_set(board_led_t led, bool state)
{
	switch(led)
	{
#ifdef LED0_PIN
		case 0:	hal_gpio_pin_set(LED0_PIN, !state);	break;
#endif
#ifdef LED1_PIN
		case 1:	hal_gpio_pin_set(LED1_PIN, !state);	break;
#endif
	}
}


#if defined BOARD_LPC1766STK

//#include <support/lcd/lcd.h>
//#define LCD_CS_PORT LPC_GPIO1	// CS_UEXT = P1.26
//#define LCD_CS_MASK (1<<26)

////#define LCD_A0_PORT LPC_GPIO4	// TXD3 = P4.28
////#define LCD_A0_MASK (1<<28)
//#define LCD_A0_PORT LPC_GPIO0	// MISO1 = P0.8
//#define LCD_A0_MASK (1<<8)

//#define LCD_RESET_PORT LPC_GPIO4	// RXD3 = P4.29
//#define LCD_RESET_MASK (1<<29)

//void lcdcon_gpo_initialize()
//{
//	pincon_setfunc(1, 26, 0, PINMODE_PULLUP);	// CS
////	pincon_setfunc(4, 28, 0, PINMODE_PULLUP);	// A0
//	pincon_setfunc(0, 8, 0, PINMODE_PULLUP);	// A0
//	pincon_setfunc(4, 29, 0, PINMODE_PULLUP);	// RESET

//	LCD_CS_PORT->FIOSET = LCD_CS_MASK;
//	LCD_CS_PORT->FIODIR |= LCD_CS_MASK;
//	LCD_A0_PORT->FIOSET = LCD_A0_MASK;
//	LCD_A0_PORT->FIODIR |= LCD_A0_MASK;
//	LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
//	LCD_RESET_PORT->FIODIR |= LCD_RESET_MASK;
//}

//void lcdcon_gpo(LCDCON_GPO gpo)
//{
//	if (gpo & LCDCON_GPO_CS) LCD_CS_PORT->FIOCLR = LCD_CS_MASK;
//	else LCD_CS_PORT->FIOSET = LCD_CS_MASK;
//	if (gpo & LCDCON_GPO_A0) LCD_A0_PORT->FIOCLR = LCD_A0_MASK;
//	else LCD_A0_PORT->FIOSET = LCD_A0_MASK;
//	if (gpo & LCDCON_GPO_RESET) LCD_RESET_PORT->FIOCLR = LCD_RESET_MASK;
//	else LCD_RESET_PORT->FIOSET = LCD_RESET_MASK;
//}

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

