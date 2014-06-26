#include <support/board_hal.h>
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>

static void _setup_i2c(int unit);
static void _setup_ssp(int unit);
static void _setup_usbhost();
static void _setup_usbdev();
static void _setup_can(int unit);
static void _setup_uart(int unit);
static void _setup_adc();

void hal_board_initialize()
{
	dma_initialize();

#if defined BOARD_MINILCD

	_setup_usbhost();
	_setup_ssp(0);
	_setup_adc();
	_setup_can(1);
	LPC_GPIO1->FIOSET = (1<<18);
	LPC_GPIO1->FIODIR |= (1<<18); // USB_LED

#elif defined BOARD_LPC1766STK

	_setup_usbhost();

	LPC_GPIO1->FIODIR |= (1<<25);	// LED1
	LPC_GPIO0->FIODIR |= (1<<4);	// LED2
	hal_led_set(0, 0);
	hal_led_set(1, 0);

#elif defined BOARD_LANDTIGER

	_setup_usbhost();
	_setup_can(0);
	_setup_can(1);

	LPC_GPIO2->FIODIR |= 0xFF;	// all 8 lower bits are outputs

#elif defined BOARD_NANO10

	_setup_usbdev();
	_setup_adc();
	_setup_can(1);

	PINSEL4bits.P2_0 = 1; // PWM1.1
	PINSEL4bits.P2_1 = 1; // PWM1.2
	PINSEL4bits.P2_2 = 1; // PWM1.3
   	PINSEL4bits.P2_3 = 1; // PWM1.4
	PINSEL4bits.P2_4 = 1; // PWM1.5
	PINSEL4bits.P2_5 = 1; // PWM1.6

	LPC_GPIO2->FIODIR |= (1<<6);	// STATUS_LED
	LPC_GPIO1->FIODIR |= (1<<18);	// USB_LED
	LPC_GPIO3->FIODIR |= (1<<25);	// GPS_LED

#else
#error Unsupported Board
#endif
}

static void _setup_i2c(int unit)
{
	switch(unit)
	{
		case 0:
			PINSEL1bits.P0_27 = 1; // SDA0
			PINSEL1bits.P0_28 = 1; // SCL0
			break;
		case 1:
			PINSEL1bits.P0_19 = 3; // SDA1
			PINSEL1bits.P0_20 = 3; // SCL1
			break;
		case 2:
			PINSEL0bits.P0_10 = 2; // SDA2
			PINSEL0bits.P0_11 = 2; // SCL2
			break;
	}
}

static void _setup_ssp(int unit)
{
	switch(unit)
	{
		case 0:
#if defined BOARD_MINILCD
			PINSEL0bits.P0_15 = 2; // SCK0
			PINSEL1bits.P0_16 = 2; // SSEL0
			PINSEL1bits.P0_17 = 2; // MISO0
			PINSEL1bits.P0_18 = 2; // MOSI0
#else
			PINSEL3bits.P1_20 = 3; // SCK0
			PINSEL3bits.P1_21 = 3; // SSEL0
			PINSEL3bits.P1_23 = 3; // MISO0
			PINSEL3bits.P1_24 = 3; // MOSI0
#endif
			break;
		case 1:
			PINSEL0bits.P0_6 = 2; // SSEL1
			PINSEL0bits.P0_7 = 2; // SCK1
			PINSEL0bits.P0_8 = 2; // MISO1
			PINSEL0bits.P0_9 = 2; // MOSI1
			break;
	}
}

static void _setup_usbhost()
{
#if defined BOARD_LPC1766STK
	PINSEL1bits.P0_29 = 1;		// D+
	PINSEL1bits.P0_30 = 1;		// D-
	PINSEL3bits.P1_18 = 1;		// USB_UP_LED
	PINSEL3bits.P1_19 = 2;		// _USB_PPWR
	PINSEL3bits.P1_22 = 2;		// USB_PWRD
	PINSEL3bits.P1_27 = 2;		// _USB_OVRCR
#endif
}

static void _setup_usbdev()
{
	PINSEL1bits.P0_29 = 1;		// D+
	PINSEL1bits.P0_30 = 1;		// D-
	PINSEL4bits.P2_9 = 1;		// USB_CONNECT
}

static void _setup_can(int unit)
{
#if defined BOARD_MINILCD 
	switch(unit)
	{
		case 1:
			PINSEL4bits.P2_7 = 1;	// RD2
			PINSEL4bits.P2_8 = 1;	// TD2
			break;
	}
#elif defined BOARD_LANDTIGER 
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_0 = 1; // RD1
			PINSEL0bits.P0_1 = 1; // TD1
			break;
		case 1:
			PINSEL0bits.P0_4 = 2; // RD2
			PINSEL0bits.P0_5 = 2; // TD2
			break;
	}
#elif defined BOARD_NANO10
	if (unit == 1)
	{
		PINSEL0bits.P0_4 = 2; // RD2
		PINSEL0bits.P0_5 = 2; // TD2
	}
#endif
}

static void _setup_uart(int unit)
{
#if defined BOARD_LANDTIGER || defined BOARD_NANO10
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_2 = 1; // select TXD0
			PINSEL0bits.P0_3 = 1; // select RXD0
			break;
		case 1:
			PINSEL0bits.P0_15 = 1; // select TXD1
			PINSEL1bits.P0_16 = 1; // select RXD1
			break;
	}
#endif
}

static void _setup_adc()
{
#if defined BOARD_MINILCD
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
	PINSEL3bits.P1_30 = 3; // AN4
	//PINSEL3bits.P1_31 = 3; // AN5
	PINSEL0bits.P0_2 = 2; // AN6
	PINSEL0bits.P0_3 = 2; // AN7
#elif defined BOARD_LANDTIGER
	PINSEL3bits.P1_31 = 3; // AN5
#elif defined BOARD_LPC1766STK
	PINSEL3bits.P1_31 = 3; // AN5
#elif defined BOARD_NANO10
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
#else
#error "Unsupported board"
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

