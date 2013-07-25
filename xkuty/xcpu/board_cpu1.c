#include <assert.h>
#include <support/board_hal.h>
#include <support/lpc11/cpu.h>
#include "board.h"


#if defined BOARD_XKUTYCPU1

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<7)
#define TAILL_MASK (1<<0)
#define BRAKEL_MASK (1<<6)
#define HORN_MASK (1<<8)
#define SENSOREN_MASK (1<<10)
#define OUTPUT_MASK (HEADL_MASK | TAILL_MASK | BRAKEL_MASK | HORN_MASK | SENSOREN_MASK)

#define MOTOR_PORT LPC_GPIO1
#define EBRAKE_MASK (1<<2)

#define LED_PORT LPC_GPIO3
#define LED_MASK (1<<0)

#define INPUT_PORT LPC_GPIO3
#define INPUT_BUTTON_MASK (1<<1)
#define INPUT_OUTPUT_ERROR_MASK (1<<2)
#define INPUT_SENSE_ERROR_MASK (1<<3)

#endif

void hal_board_initialize()
{
#if defined BOARD_XKUTYCPU1
	LED_PORT->DIR |= LED_MASK;
	LED_PORT->MASKED_ACCESS[LED_MASK] = 0;	// led on

	OUTPUT_PORT->DIR |= OUTPUT_MASK;
	OUTPUT_PORT->MASKED_ACCESS[OUTPUT_MASK] = 0;

	MOTOR_PORT->DIR |= EBRAKE_MASK;
	MOTOR_PORT->MASKED_ACCESS[EBRAKE_MASK] = EBRAKE_MASK;	// active low

	LPC_IOCON->PIO1_5 = 2 | (2<<3) | (1<<5);	// T32B0.CAP0
	LPC_IOCON->R_PIO1_1 = 3 | (1<<7);	// T32B1.MAT0
	LPC_IOCON->R_PIO1_2 = 1 | (1<<3);	// PIO1_2 

#elif defined BOARD_MIORELAY1
	// enable CAN term
	LPC_GPIO2->DIR |= 1<<8;
	LPC_GPIO2->MASKED_ACCESS[1<<8] = 1<<8;
#else
#error Unsupported Board
#endif
}

void xcpu_board_output(XCPU_OUTPUT_MASK mask)
{
	OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = (mask & OUTPUT_HEADL) ? HEADL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[TAILL_MASK] = (mask & OUTPUT_TAILL) ? TAILL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[BRAKEL_MASK] = (mask & OUTPUT_BRAKEL) ? BRAKEL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = (mask & OUTPUT_HORN) ? HORN_MASK : 0;
	MOTOR_PORT->MASKED_ACCESS[EBRAKE_MASK] = (mask & OUTPUT_EBRAKE) ? 0 : EBRAKE_MASK;	// active low
}

XCPU_INPUT_MASK xcpu_board_input(XCPU_INPUT_MASK mask)
{
	XCPU_INPUT_MASK read = INPUT_NONE;
	unsigned long input = INPUT_PORT->DATA;

	if ((mask & INPUT_BUTTON_START) &&
		!(input & INPUT_BUTTON_MASK)) read |= INPUT_BUTTON_START;

	return read;
}

void xcpu_board_led(int led)
{
	LED_PORT->MASKED_ACCESS[LED_MASK] = led ? 0 : LED_MASK;	// led is active low
}

static int _setup_adc(int unit)
{
	unsigned adc_conf = 2 | //	Func. AD
						(0<<3) | // No pull-up or pull-down
						(0<<5) | // No hysteresis
						(0<<7) | // Analog input
						(0<<10); // Standar GPIO output

	LPC_IOCON->R_PIO0_11 = adc_conf; // AD0
	LPC_IOCON->R_PIO1_0 = adc_conf;	// AD1
	//LPC_IOCON->R_PIO1_1 = adc_conf;	// AD2
	//LPC_IOCON->R_PIO1_2 = adc_conf;	// AD3
	//LPC_IOCON->SWDIO_PIO1_3 = adc_conf;	// AD4
	//LPC_IOCON->PIO1_4 = adc_conf;	// AD5
	LPC_IOCON->PIO1_10 = adc_conf; // AD6
	LPC_IOCON->PIO1_11 = adc_conf; // AD7

	return 0xc3;	// 0,1,6,7
}


int hal_board_init_pinmux(HAL_RESOURCE res, int unit)
{
	switch(res)
	{
		case HAL_RESOURCE_ADC: return _setup_adc(unit);
		default:
			assert(0);
	}
	return 0;
}

