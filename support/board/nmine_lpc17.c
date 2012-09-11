#include <support/board_hal.h>
#include <support/lpc17/pincon.h>

static int _setup_i2c(int unit);
static int _setup_ssp(int unit);
static int _setup_usbdev(int unit);
static int _setup_pwm(int unit);
static int _setup_cap(int unit);
static int _setup_mat(int unit);
static int _setup_can(int unit);
static int _setup_uart(int unit);
static int _setup_adc(int unit);

void hal_board_initialize()
{
#ifdef BOARD_NANO10
    LPC_GPIO2->FIODIR |= (1<<6); // STATUS_LED
	LPC_GPIO1->FIODIR |= (1<<18); // USB_LED
	LPC_GPIO3->FIODIR |= (1<<25); // GPS_LED 
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

static int _setup_usbdev(int unit)
{
	PINSEL1bits.P0_29 = 1;		// D+
	PINSEL1bits.P0_30 = 1;		// D-
	PINSEL4bits.P2_9 = 1;		// USB_CONNECT
	return 1;
}

static int _setup_pwm(int unit)
{
#if defined(BOARD_NANO10) 
	PINSEL4bits.P2_0 = 1; // PWM1.1
	PINSEL4bits.P2_1 = 1; // PWM1.2
	PINSEL4bits.P2_2 = 1; // PWM1.3
   	PINSEL4bits.P2_3 = 1; // PWM1.4
	PINSEL4bits.P2_4 = 1; // PWM1.5
	PINSEL4bits.P2_5 = 1; // PWM1.6
	return 0x3f;
#else
#error "Unsupported board"
#endif
}

static int _setup_cap(int unit)
{
#if defined BOARD_NANO10
	if (unit == 0)
	{
		PINSEL3bits.P1_26 = 3; // CAP0.0
		return 1;
	}
#else
#error "Unsupported board"
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
#else
#error "Unsupported board"
#endif
	return 0;
}

static int _setup_can(int unit)
{
#if defined(BOARD_NANO10)
	switch(unit)
	{
		case 1:
			PINSEL0bits.P0_4 = 2;	// RD2
			PINSEL0bits.P0_5 = 2;	// TD2
			return 1;
	}
#else
#error "Unsupported board"
#endif
	return 0;
}

static int _setup_uart(int unit)
{
#if defined(BOARD_NANO10)
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
#else
#error "Unsupported board"
#endif
	return 0;
}

static int _setup_adc(int unit)
{
	unsigned char ch_mask = 0;
#if defined(BOARD_NANO10)
	PINSEL1bits.P0_23 = 1; // AN0
	PINSEL1bits.P0_24 = 1; // AN1
	PINSEL1bits.P0_25 = 1; // AN2
	PINSEL1bits.P0_26 = 1; // AN3
	PINSEL3bits.P1_30 = 3; // AN4
	PINSEL3bits.P1_31 = 3; // AN5
	ch_mask = 0x3f; // six inputs
#else
#error "Unsupported board"
#endif
	return ch_mask;
}

void hal_led_set(HAL_LED led, int state)
{
	switch(led)
	{
		case 0:
		case LED_STATUS:
			if (state) 
				LPC_GPIO2->FIOSET = (1<<6);	// STATUS_LED
			else
				LPC_GPIO2->FIOCLR = (1<<6);	// STATUS_LED
			break;
		case 1:
		case LED_SDCARD:
			if (state)
				LPC_GPIO1->FIOSET = (1<<18); // USB_LED
			else
				LPC_GPIO1->FIOCLR = (1<<18); // USB_LED
			break;
		case 2:
		case LED_GPS:
			if (state)
				LPC_GPIO3->FIOSET = (1<<25); // GPS_LED 
			else
				LPC_GPIO3->FIOCLR = (1<<25); // GPS_LED 
			break;
	}
}

#if defined BOARD_NANO10 && defined BOARD_ENABLE_SD_CARD_SUPPORT

#include <support/misc/sdcard_spi.h>

#define CARD_POWER_MASK (1<<21)
#define CARD_POWER_PORT LPC_GPIO0
#define CARD_POWER_ON	CARD_POWER_PORT->FIOCLR = CARD_POWER_MASK
#define CARD_POWER_OFF	CARD_POWER_PORT->FIOSET = CARD_POWER_MASK

void sd_spi_power_control(int power)
{
	if (power)
	{
    	CARD_POWER_OFF;
		CARD_POWER_PORT->FIODIR |= CARD_POWER_MASK;
		for(volatile int i = 0; i < 100000; i++);
		CARD_POWER_ON;
	}
	else
	{
		CARD_POWER_OFF;
	}
}

#endif