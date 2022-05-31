#include <support/board_hal.h>

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
#define LCD_A0_PORT LPC_GPIO4	// TXD3 = P4.28
#define LCD_A0_MASK (1<<28)
#define LCD_RESET_PORT LPC_GPIO4	// RXD3 = P4.29
#define LCD_RESET_MASK (1<<29)

void lcdcon_gpo_initialize()
{
	PINSEL3bits.P1_26 = 0;
	PINSEL9bits.P4_28 = 0;
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

