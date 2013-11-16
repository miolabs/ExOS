#include <support/board_hal.h>
#include <support/i2c_hal.h>
#include <CMSIS/LPC11xx.h>

static int _setup_i2c(int unit);
static int _setup_ssp(int unit);
static int _setup_cap(int unit);
static int _setup_mat(int unit);
static int _setup_uart(int unit);
static int _setup_adc(int unit);

void hal_board_initialize()
{
	hal_i2c_initialize(0, 400000);
}

int hal_board_init_pinmux(HAL_RESOURCE res, int unit)
{
	switch(res)
	{
		case HAL_RESOURCE_I2C:		return _setup_i2c(unit);
		case HAL_RESOURCE_SSP:		return _setup_ssp(unit);
		case HAL_RESOURCE_CAP:		return _setup_cap(unit);
		case HAL_RESOURCE_MAT:		return _setup_mat(unit);
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
			LPC_IOCON->PIO0_4 = 1; // SCL
			LPC_IOCON->PIO0_5 = 1; // SDA
			return 1;
	}
}

static int _setup_ssp(int unit)
{
	switch(unit)
	{
		case 0:
//			PINSEL3bits.P1_20 = 3; // SCK0
//			PINSEL3bits.P1_21 = 3; // SSEL0
//			PINSEL3bits.P1_23 = 3; // MISO0
//			PINSEL3bits.P1_24 = 3; // MOSI0
			return 1;
		case 1:
			return 1;
	}
}

static int _setup_cap(int unit)
{
	return 0;
}

static int _setup_mat(int unit)
{
	return 0;
}

static int _setup_uart(int unit)
{
	return 0;
}

static int _setup_adc(int unit)
{
	unsigned char ch_mask = 0;
	LPC_IOCON->R_PIO0_11 = 2; // AD0
	LPC_IOCON->R_PIO1_0 = 2; // AD1
	LPC_IOCON->R_PIO1_1 = 2; // AD2
	LPC_IOCON->R_PIO1_2 = 2; // AD3
	ch_mask |= 0xf;
	return ch_mask;
}


