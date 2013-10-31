#include <support/board_hal.h>
#include <support/lpc17/dma.h>
#include <support/lpc17/pincon.h>

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
	dma_initialize();

#if defined BOARD_MIOBOARD
#define LED1_PORT LPC_GPIO3
#define LED1_MASK (1<<23)
	LED1_PORT->CLR = LED1_MASK;
#elif defined BOARD_UMFI
	// NO USER LEDS
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
			LPC_GPIO0->DIR |= (1<<27);
			LPC_GPIO0->SET = (1<<27);
			LPC_GPIO0->CLR = (1<<27);
			
			pincon_setfunc(0, 27, 1);	// SDA0
			pincon_setfunc(0, 28, 1);	// SCL0
			return 1;
		case 1:
			pincon_setfunc(0, 19, 3);	// SDA1
			pincon_setfunc(0, 20, 3);	// SCL1
			return 1;
		case 2:
			pincon_setfunc(0, 10, 2);	// SDA2
			pincon_setfunc(0, 11, 2);	// SCL2
			return 1;
	}
}

static int _setup_ssp(int unit)
{
	switch(unit)
	{
		case 0:
//			PINSEL0bits.P0_15 = 2; // SCK0
//			PINSEL1bits.P0_16 = 2; // SSEL0
//			PINSEL1bits.P0_17 = 2; // MISO0
//			PINSEL1bits.P0_18 = 2; // MOSI0
			return 1;
		case 1:
//			PINSEL0bits.P0_6 = 2; // SSEL1
//			PINSEL0bits.P0_7 = 2; // SCK1
//			PINSEL0bits.P0_8 = 2; // MISO1
//			PINSEL0bits.P0_9 = 2; // MOSI1
			return 1;
	}
}

static int _setup_usbhost(int unit)
{
#if defined BOARD_MIOBOARD
	// usb1
	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2);	// USB_PWRD1
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1);	// D+ 
	pincon_setfunc(0, 12, 1);	// _USB_PPWR2
	return (1<<0) | (1<<1);
#elif defined BOARD_UMFI
	// usb1
	pincon_setfunc(0, 29, 1);	// D+
	pincon_setfunc(0, 30, 1);	// D-
	pincon_setfunc(1, 18, 1);	// USB_UP_LED1
	pincon_setfunc(1, 19, 2);	// _USB_PPWR1
	pincon_setfunc(1, 22, 2);	// USB_PWRD1
	pincon_setfunc(1, 27, 2);	// _USB_OVRCR1
	
	// usb2
	pincon_setfunc(0, 31, 1);	// D+
	pincon_setfunc(0, 13, 1);	// USB_UP_LED2
	pincon_setfunc(0, 12, 1);	// _USB_PPWR2
	pincon_setfunc(1, 30, 1);	// USB_PWRD2
	pincon_setfunc(1, 31, 1);	// _USB_OVRCR2
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
	return 0;
}

static int _setup_pwm(int unit)
{
#if defined BOARD_MIOBOARD || defined BOARD_UMFI 
//	PINSEL4bits.P2_0 = 1; // PWM1.1
//	PINSEL4bits.P2_1 = 1; // PWM1.2
//	PINSEL4bits.P2_2 = 1; // PWM1.3
//   	PINSEL4bits.P2_3 = 1; // PWM1.4
//	PINSEL4bits.P2_4 = 1; // PWM1.5
//	PINSEL4bits.P2_5 = 1; // PWM1.6
	return 0;
#else
#error "Unsupported board"
#endif
}

static int _setup_cap(int unit)
{
	return 0;
}

static int _setup_mat(int unit)
{
	return 0;
}

static int _setup_can(int unit)
{
#if defined BOARD_MIOBOARD 
	switch(unit)
	{
		case 0:
			pincon_setfunc(0, 0, 1);	// CAN_RD1
			pincon_setfunc(0, 1, 1);	// CAN_TD1 
			return 1;
		case 1:
			pincon_setfunc(2, 7, 1);	// CAN_RD2
			pincon_setfunc(2, 8, 1);	// CAN_TD2 
			return 1;
	}
#elif defined BOARD_UMFI
	return 0;
#else
#error "Unsupported board"
#endif
	return 0;
}

static int _setup_uart(int unit)
{
#if defined BOARD_MIOBOARD || defined BOARD_UMFI
	return 0;
#else
#error "Unsupported board"
#endif
	return 0;
}

static int _setup_adc(int unit)
{
	unsigned char ch_mask = 0;
#if defined BOARD_MIOBOARD || defined BOARD_UMFI
#else
#error "Unsupported board"
#endif
	return ch_mask;
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




