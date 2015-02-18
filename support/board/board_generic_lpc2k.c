#include <support/board_hal.h>
#include <support/lpc2k/pincon.h>
#include <support/lpc2k/dma.h>

static void _setup_i2c(int unit);
static void _setup_ssp(int unit);
static void _setup_usbhost();
static void _setup_usbdev();
static void _setup_can(int unit);
static void _setup_uart(int unit);
static void _setup_adc();
static void _setup_mci();

void hal_board_initialize()
{
	LPC_SC->SCS |= SCS_GPIOM;
	dma_initialize();

#if defined BOARD_EA2478_MOD

	_setup_usbhost();
	_setup_can(0);
	_setup_can(1);
	_setup_uart(0);
	_setup_uart(1);
    _setup_adc();
    _setup_mci();
 
	LPC_GPIO2->FIODIR |= (1<<6);	// LED_GPS
	LPC_GPIO2->FIODIR |= (1<<9);	// LED_SD

#elif defined BOARD_E2468

	_setup_usbhost();
	_setup_can(0);
	_setup_can(1);
	_setup_uart(0);
    _setup_adc();
    _setup_mci();

	LPC_GPIO4->FIODIR |= (1<<17);	// STAT1
	hal_led_set(0, 0);
	LPC_GPIO4->FIODIR |= (1<<16);	// STAT2
	hal_led_set(1, 0);

#elif defined BOARD_CR2

	_setup_usbhost();
	_setup_can(0);
	_setup_can(1);
    _setup_adc();
	_setup_mci();

	LPC_GPIO3->FIODIR |= (1<<16);	// CAN_LED
	hal_led_set(LED_CAN, 0);
	LPC_GPIO3->FIODIR |= (1<<17);	// SD_LED
	hal_led_set(LED_SDCARD, 0);
	LPC_GPIO3->FIODIR |= (1<<18);	// GPS_LED
	hal_led_set(LED_GPS, 0);
	LPC_GPIO4->FIODIR |= (1<<31);	// USB_LED
	hal_led_set(LED_USB, 0);

#elif defined BOARD_ICDEV_LPC2478

    _setup_mci();
	LPC_GPIO0->FIODIR |= (1<<13);	// USB_LED
	hal_led_set(LED_USB, 0);

#else
#error Unsupported Board
#endif
}

static void _setup_i2c(int unit)
{
	switch(unit)
	{
		case 0:
			pincon_setfunc(0, 27, 1, PINMODE_PULLUP); // SDA0
			pincon_setfunc(0, 28, 1, PINMODE_PULLUP); // SCL0
			break;
		case 1:
			pincon_setfunc(0, 19, 3, PINMODE_PULLUP); // SDA1
			pincon_setfunc(0, 20, 3, PINMODE_PULLUP); // SCL1
			break;
		case 2:
			pincon_setfunc(0, 10, 2, PINMODE_PULLUP); // SDA2
			pincon_setfunc(0, 11, 2, PINMODE_PULLUP); // SCL2
			break;
	}
}

static void _setup_ssp(int unit)
{
	switch(unit)
	{
		case 0:
			pincon_setfunc(1, 20, 3, PINMODE_PULLUP); // SCK0
			pincon_setfunc(1, 21, 3, PINMODE_PULLUP); // SSEL0
			pincon_setfunc(1, 23, 3, PINMODE_PULLUP); // MISO0
			pincon_setfunc(1, 24, 3, PINMODE_PULLUP); // MOSI0
			break;
		case 1:
			pincon_setfunc(0, 6, 2, PINMODE_PULLUP); // SSEL1
			pincon_setfunc(0, 7, 2, PINMODE_PULLUP); // SCK1
			pincon_setfunc(0, 8, 2, PINMODE_PULLUP); // MISO1
			pincon_setfunc(0, 9, 2, PINMODE_PULLUP); // MOSI1
			break;
	}
}

static void _setup_usbhost()
{
#if defined BOARD_E2468 || defined BOARD_EA2478_MOD || defined BOARD_CR2
	pincon_setfunc(0, 29, 1, PINMODE_PULLUP);	// P0.29 = USB_D+1
	pincon_setfunc(0, 30, 1, PINMODE_PULLUP);	// P0.30 = USB_D-1
	pincon_setfunc(0, 31, 1, PINMODE_PULLUP);	// P0.31 = USB_D+2
	pincon_setfunc(1, 19, 2, PINMODE_PULLUP);	// P1.19 = _USB_PPWR1
	pincon_setfunc(1, 22, 2, PINMODE_PULLUP);	// P1.22 = USB_PWRD1
	pincon_setfunc(1, 27, 2, PINMODE_PULLUP);	// P1.27 = _USB_OVRCR1
	pincon_setfunc(0, 12, 1, PINMODE_PULLUP);	// P0.12 = _USB_PPWR2
	pincon_setfunc(1, 30, 1, PINMODE_PULLUP);	// P1.30 = USB_PWRD2
	pincon_setfunc(1, 31, 1, PINMODE_PULLUP);	// P1.31 = _USB_OVRCR2
	#ifndef USB_HOST_NO_LEDS
	pincon_setfunc(1, 18, 1, PINMODE_PULLUP);	// P1.18 = USB_UP_LED1 __opt
	pincon_setfunc(0, 13, 1, PINMODE_PULLUP);	// P0.13 = USB_UP_LED2 __opt
	#endif
#endif
}

static void _setup_usbdev()
{
//	PINSEL1bits.P0_29 = 1;		// D+
//	PINSEL1bits.P0_30 = 1;		// D-
//	PINSEL4bits.P2_9 = 1;		// USB_CONNECT
}

static void _setup_can(int unit)
{
#if defined BOARD_EA2478_MOD || defined BOARD_CR2
	switch(unit)
	{
		case 0:
			pincon_setfunc(0, 0, 1, PINMODE_PULLUP); // RD1
			pincon_setfunc(0, 1, 1, PINMODE_PULLUP); // TD1
			break;
		case 1:
			pincon_setfunc(0, 4, 2, PINMODE_PULLUP); // RD2
			pincon_setfunc(0, 5, 2, PINMODE_PULLUP); // TD2
			break;
	}
#endif
}

static void _setup_uart(int unit)
{
#if defined BOARD_EA2478_MOD
	switch(unit)
	{
		case 0:
			pincon_setfunc(0, 2, 1, PINMODE_PULLUP); // select TXD0
			pincon_setfunc(0, 3, 1, PINMODE_PULLUP); // select RXD0
			break;
		case 1:
			pincon_setfunc(0, 15, 1, PINMODE_PULLUP); // select TXD1
			pincon_setfunc(0, 16, 1, PINMODE_PULLUP); // select RXD1
			break;
	}
#elif defined BOARD_E2468
	switch(unit)
	{
		case 0:
			pincon_setfunc(0, 2, 1, PINMODE_PULLUP); // select TXD0
			pincon_setfunc(0, 3, 1, PINMODE_PULLUP); // select RXD0
			break;
	}
#endif
}

static void _setup_adc()
{
#if defined BOARD_EA2478_MOD
	pincon_setfunc(0, 23, 1, PINMODE_PULLUP); // AN0
	pincon_setfunc(0, 24, 1, PINMODE_PULLUP); // AN1
	pincon_setfunc(0, 25, 1, PINMODE_PULLUP); // AN2
	pincon_setfunc(0, 26, 1, PINMODE_PULLUP); // AN3
	pincon_setfunc(1, 30, 3, PINMODE_PULLUP); // AN4
	pincon_setfunc(1, 31, 3, PINMODE_PULLUP); // AN5
#elif defined BOARD_E2468
	pincon_setfunc(0, 23, 1, PINMODE_PULLUP); // AN0
	pincon_setfunc(0, 24, 1, PINMODE_PULLUP); // AN1
	pincon_setfunc(0, 25, 1, PINMODE_PULLUP); // AN2
	pincon_setfunc(0, 26, 1, PINMODE_PULLUP); // AN3
#elif defined BOARD_CR2
	pincon_setfunc(0, 23, 1, PINMODE_PULLUP); // AN0
	pincon_setfunc(0, 24, 1, PINMODE_PULLUP); // AN1
	pincon_setfunc(0, 25, 1, PINMODE_PULLUP); // AN2
	pincon_setfunc(0, 26, 1, PINMODE_PULLUP); // AN3
	pincon_setfunc(1, 30, 3, PINMODE_PULLUP); // AN4
	pincon_setfunc(1, 31, 3, PINMODE_PULLUP); // AN5
	pincon_setfunc(0, 12, 3, PINMODE_PULLUP); // AN6
	pincon_setfunc(0, 13, 3, PINMODE_PULLUP); // AN7
#endif
}

static void _setup_mci()
{
#if defined BOARD_ICDEV_LPC2478
	pincon_setfunc(1, 2, 2, PINMODE_PULLUP); // MCICLK
	pincon_setfunc(1, 3, 2, PINMODE_PULLUP); // MCICMD
	pincon_setfunc(1, 5, 2, PINMODE_PULLUP); // MCIPWR
	pincon_setfunc(1, 6, 2, PINMODE_PULLUP); // MCIDAT0
	pincon_setfunc(1, 7, 2, PINMODE_PULLUP); // MCIDAT1
	pincon_setfunc(1, 11, 2, PINMODE_PULLUP); // MCIDAT2
	pincon_setfunc(1, 12, 2, PINMODE_PULLUP); // MCIDAT3
	LPC_SC->SCS &= ~SCS_MCIPWR; // MCIPWR Active Low
#elif defined BOARD_E2468
	pincon_setfunc(1, 2, 2, PINMODE_PULLUP); // MCICLK
	pincon_setfunc(1, 3, 2, PINMODE_PULLUP); // MCICMD
	pincon_setfunc(1, 5, 2, PINMODE_PULLUP); // MCIPWR
	pincon_setfunc(1, 6, 2, PINMODE_PULLUP); // MCIDAT0
	pincon_setfunc(1, 7, 2, PINMODE_PULLUP); // MCIDAT1
	pincon_setfunc(1, 11, 2, PINMODE_PULLUP); // MCIDAT2
	pincon_setfunc(1, 12, 2, PINMODE_PULLUP); // MCIDAT3
	LPC_SC->SCS |= SCS_MCIPWR; // MCIPWR Active High
#endif
}

#if defined BOARD_EA2478_MOD
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_GPS:
		case 0:
			if (state) LPC_GPIO2->FIOCLR = (1<<6);	// LED_GPS
			else LPC_GPIO2->FIOSET = (1<<6);	// LED_GPS
			break;
		case LED_SDCARD:
		case 1:
			if (state) LPC_GPIO2->FIOCLR = (1<<9); // LED_SD
			else LPC_GPIO2->FIOSET = (1<<9); // LED_SD
			break;
	}
}
#endif

#if defined BOARD_E2468
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
			if (state) LPC_GPIO4->FIOCLR = (1<<17);
			else LPC_GPIO4->FIOSET = (1<<17);
			break;
		case 1:
			if (state) LPC_GPIO4->FIOCLR = (1<<16);
			else LPC_GPIO4->FIOSET = (1<<16);
			break;
	}
}
#endif

#if defined BOARD_CR2
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_CAN:
		case 0:
			if (state) LPC_GPIO3->FIOCLR = (1<<16);
			else LPC_GPIO3->FIOSET = (1<<16);
			break;
		case LED_SDCARD:
		case 1:
			if (state) LPC_GPIO3->FIOCLR = (1<<17);
			else LPC_GPIO3->FIOSET = (1<<17);
			break;
		case LED_GPS:
		case 2:
			if (state) LPC_GPIO3->FIOCLR = (1<<18);
			else LPC_GPIO3->FIOSET = (1<<18);
			break;
		case LED_USB:
		case 3:
			if (state) LPC_GPIO4->FIOCLR = (1<<31);
			else LPC_GPIO4->FIOSET = (1<<31);
			break;
	}
}
#endif

#if defined BOARD_ICDEV_LPC2478
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case LED_USB:
		case 0:
			if (state) LPC_GPIO0->FIOCLR = (1<<13);
			else LPC_GPIO0->FIOSET = (1<<13);
			break;
	}
}
#endif




