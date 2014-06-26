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
			PINSEL3bits.P1_20 = 3; // SCK0
			PINSEL3bits.P1_21 = 3; // SSEL0
			PINSEL3bits.P1_23 = 3; // MISO0
			PINSEL3bits.P1_24 = 3; // MOSI0
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
#if defined BOARD_E2468 || defined BOARD_EA2478_MOD || defined BOARD_CR2
	PINSEL1bits.P0_29 = 1;	// P0.29 = USB_D+1
	PINSEL1bits.P0_30 = 1;	// P0.30 = USB_D-1
	PINSEL1bits.P0_31 = 1;	// P0.31 = USB_D+2
	PINSEL3bits.P1_19 = 2;	// P1.19 = _USB_PPWR1
	PINSEL3bits.P1_22 = 2;	// P1.22 = USB_PWRD1
	PINSEL3bits.P1_27 = 2;	// P1.27 = _USB_OVRCR1
	PINSEL0bits.P0_12 = 1;	// P0.12 = _USB_PPWR2
	PINSEL3bits.P1_30 = 1;	// P1.30 = USB_PWRD2
	PINSEL3bits.P1_31 = 1;	// P1.31 = _USB_OVRCR2
	#ifndef USB_HOST_NO_LEDS
	PINSEL3bits.P1_18 = 1;	// P1.18 = USB_UP_LED1 __opt
	PINSEL0bits.P0_13 = 1;	// P0.13 = USB_UP_LED2 __opt
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
			PINSEL0bits.P0_0 = 1; // RD1
			PINSEL0bits.P0_1 = 1; // TD1
			break;
		case 1:
			PINSEL0bits.P0_4 = 2; // RD2
			PINSEL0bits.P0_5 = 2; // TD2
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
			PINSEL0bits.P0_2 = 1; // select TXD0
			PINSEL0bits.P0_3 = 1; // select RXD0
			break;
		case 1:
			PINSEL0bits.P0_15 = 1; // select TXD1
			PINSEL1bits.P0_16 = 1; // select RXD1
			break;
	}
#elif defined BOARD_E2468
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_2 = 1; // select TXD0
			PINSEL0bits.P0_3 = 1; // select RXD0
			break;
	}
#endif
}

static void _setup_adc()
{
#if defined BOARD_EA2478_MOD
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
	PINSEL3bits.P1_30 = 3; // AN4
	PINSEL3bits.P1_31 = 3; // AN5
#elif defined BOARD_E2468
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
#elif defined BOARD_CR2
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
	PINSEL3bits.P1_30 = 3; // AN4
	PINSEL3bits.P1_31 = 3; // AN5
	PINSEL0bits.P0_12 = 3; // AN6
	PINSEL0bits.P0_13 = 3; // AN7
#endif
}

static void _setup_mci()
{
#if defined BOARD_ICDEV_LPC2478
	PINSEL2bits.P1_2 = 2; // MCICLK
	PINSEL2bits.P1_3 = 2; // MCICMD
	PINSEL2bits.P1_5 = 2; // MCIPWR
	PINSEL2bits.P1_6 = 2; // MCIDAT0
	PINSEL2bits.P1_7 = 2; // MCIDAT1
	PINSEL2bits.P1_11 = 2; // MCIDAT2
	PINSEL2bits.P1_12 = 2; // MCIDAT3
	LPC_SC->SCS &= ~SCS_MCIPWR; // MCIPWR Active Low
#elif defined BOARD_E2468
	PINSEL2bits.P1_2 = 2; // MCICLK
	PINSEL2bits.P1_3 = 2; // MCICMD
	PINSEL2bits.P1_5 = 2; // MCIPWR
	PINSEL2bits.P1_6 = 2; // MCIDAT0
	PINSEL2bits.P1_7 = 2; // MCIDAT1
	PINSEL2bits.P1_11 = 2; // MCIDAT2
	PINSEL2bits.P1_12 = 2; // MCIDAT3
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




