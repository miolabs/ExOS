#include <support/board_hal.h>
#include <support/lpc2k/pincon.h>

static int _setup_i2c(int unit);
static int _setup_ssp(int unit);
static int _setup_usbhost(int unit);
static int _setup_usbdev(int unit);
static int _setup_pwm(int unit);
static int _setup_cap(int unit);
static int _setup_mat(int unit);
static int _setup_can(int unit);
static int _setup_uart(int unit);
static int _setup_adc(int unit);

void hal_board_initialize()
{
#if defined BOARD_EA2478_MOD
    LPC_GPIO2->FIODIR |= (1<<6);	// LED_GPS
	LPC_GPIO2->FIODIR |= (1<<9);	// LED_SD
#elif defined BOARD_E2468
    LPC_GPIO4->FIODIR |= (1<<17);	// STAT1
	hal_led_set(0, 0);
	LPC_GPIO4->FIODIR |= (1<<16);	// STAT2
	hal_led_set(1, 0);
#else
#error Unsupported Board
#endif
}

int hal_board_init_pinmux(HAL_RESOURCE res, int unit)
{
	switch(res)
	{
		case HAL_RESOURCE_I2C:		return _setup_i2c(unit);
		case HAL_RESOURCE_SSP:		return _setup_ssp(unit);
		case HAL_RESOURCE_USBHOST:	return _setup_usbhost(unit);
        case HAL_RESOURCE_USBDEV:	return _setup_usbdev(unit);
		case HAL_RESOURCE_PWM:		return _setup_pwm(unit);
		case HAL_RESOURCE_CAP:		return _setup_cap(unit);
		case HAL_RESOURCE_MAT:		return _setup_mat(unit);
		case HAL_RESOURCE_CAN:		return _setup_can(unit);
		case HAL_RESOURCE_UART:		return _setup_uart(unit);
		case HAL_RESOURCE_ADC:		return _setup_adc(unit);
	}
	return 0;
}

static int _setup_i2c(int unit)
{
	switch(unit)
	{
		case 0:
			PINSEL1bits.P0_27 = 1; // SDA0
			PINSEL1bits.P0_28 = 1; // SCL0
			return 1;
		case 1:
			PINSEL1bits.P0_19 = 3; // SDA1
			PINSEL1bits.P0_20 = 3; // SCL1
			return 1;
		case 2:
			PINSEL0bits.P0_10 = 2; // SDA2
			PINSEL0bits.P0_11 = 2; // SCL2
			return 1;
	}
}

static int _setup_ssp(int unit)
{
	switch(unit)
	{
		case 0:
			PINSEL3bits.P1_20 = 3; // SCK0
			PINSEL3bits.P1_21 = 3; // SSEL0
			PINSEL3bits.P1_23 = 3; // MISO0
			PINSEL3bits.P1_24 = 3; // MOSI0
			return 1;
		case 1:
			PINSEL0bits.P0_6 = 2; // SSEL1
			PINSEL0bits.P0_7 = 2; // SCK1
			PINSEL0bits.P0_8 = 2; // MISO1
			PINSEL0bits.P0_9 = 2; // MOSI1
			return 1;
	}
}

static int _setup_usbhost(int unit)
{
#if defined BOARD_E2468 || defined BOARD_EA2478_MOD
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
	return (1<<0) | (1<<1);
#else
	return 0;
#endif
}

static int _setup_usbdev(int unit)
{
//	PINSEL1bits.P0_29 = 1;		// D+
//	PINSEL1bits.P0_30 = 1;		// D-
//	PINSEL4bits.P2_9 = 1;		// USB_CONNECT
//	return 1;
	return 0;
}

static int _setup_pwm(int unit)
{
#if defined BOARD_NANO10 
	PINSEL4bits.P2_0 = 1; // PWM1.1
	PINSEL4bits.P2_1 = 1; // PWM1.2
	PINSEL4bits.P2_2 = 1; // PWM1.3
   	PINSEL4bits.P2_3 = 1; // PWM1.4
	PINSEL4bits.P2_4 = 1; // PWM1.5
	PINSEL4bits.P2_5 = 1; // PWM1.6
	return 0x3f;
#endif
	return 0;
}

static int _setup_cap(int unit)
{
#if defined BOARD_NANO10
	if (unit == 0)
	{
		PINSEL3bits.P1_26 = 3; // CAP0.0
		return 1;
	}
#endif
	return 0;
}

static int _setup_mat(int unit)
{
#if defined BOARD_NANO10
	if (unit == 2)
	{
		PINSEL9bits.P4_28 = 2; // MAT2.0
		PINSEL9bits.P4_29 = 2; // MAT2.1
		return 3;
	}
#endif
	return 0;
}

static int _setup_can(int unit)
{
#if defined BOARD_EA2478_MOD 
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_0 = 1; // RD1
			PINSEL0bits.P0_1 = 1; // TD1
			return 1;
		case 1:
			PINSEL0bits.P0_4 = 2; // RD2
			PINSEL0bits.P0_5 = 2; // TD2
			return 1;
	}
#endif
	return 0;
}

static int _setup_uart(int unit)
{
#if defined BOARD_EA2478_MOD
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_2 = 1; // select TXD0
			PINSEL0bits.P0_3 = 1; // select RXD0
			return 1;
		case 1:
			PINSEL0bits.P0_15 = 1; // select TXD1
			PINSEL1bits.P0_16 = 1; // select RXD1
			return 1;
	}
#elif defined BOARD_E2468
	switch(unit)
	{
		case 0:
			PINSEL0bits.P0_2 = 1; // select TXD0
			PINSEL0bits.P0_3 = 1; // select RXD0
			return 1;
	}
#endif
	return 0;
}

static int _setup_adc(int unit)
{
	unsigned char ch_mask = 0;
#if defined BOARD_EA2478_MOD
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
	PINSEL3bits.P1_30 = 3; // AN4
	PINSEL3bits.P1_31 = 3; // AN5
	ch_mask = 0x3f; // six inputs
#endif
	return ch_mask;
}

#if defined BOARD_EA2478_MOD
void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
			if (state) 
				LPC_GPIO2->FIOCLR = (1<<6);	// LED_GPS
			else
				LPC_GPIO2->FIOSET = (1<<6);	// LED_GPS
			break;
		case 1:
			if (state)
				LPC_GPIO2->FIOCLR = (1<<9); // LED_SD
			else
				LPC_GPIO2->FIOSET = (1<<9); // LED_SD
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
			if (state) 
				LPC_GPIO4->FIOCLR = (1<<17);
			else
				LPC_GPIO4->FIOSET = (1<<17);
			break;
		case 1:
			if (state)
				LPC_GPIO4->FIOCLR = (1<<16);
			else
				LPC_GPIO4->FIOSET = (1<<16);
			break;
	}
}
#endif






