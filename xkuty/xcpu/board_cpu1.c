#include <assert.h>
#include <support/board_hal.h>
#include <support/lpc11/cpu.h>
#include "board.h"


#if defined BOARD_XKUTYCPU1_PROTO

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

#elif defined BOARD_XKUTYCPU1_EXTENDED

#define OUTPUT_PORT LPC_GPIO2
#define HEADL_MASK (1<<7)
#define TAILL_MASK (1<<0)
#define BRAKEL_MASK (1<<6)
#define HORN_MASK (1<<8)
#define AUX_MASK (1<<13)
#define LEFTL_MASK (1<<2)
#define RIGHTL_MASK (1<<3)
#define OUTPUT_MASK (HEADL_MASK | TAILL_MASK | BRAKEL_MASK | HORN_MASK | AUX_MASK | LEFTL_MASK | RIGHTL_MASK)
#define OUTPUT_IDLE_MASK (0)

#define OUTPUT2_PORT LPC_GPIO1
#define EBRAKE_MASK (1<<2)
#define MOTOREN_MASK (1<<8)
#define HIGHBL_MASK (1<<11)
#define OUTPUT2_MASK (EBRAKE_MASK | MOTOREN_MASK | HIGHBL_MASK)
#define OUTPUT2_IDLE_MASK (EBRAKE_MASK)

#define OUTPUT3_PORT LPC_GPIO0
#define POWEREN_MASK (1<<3)
#define LED_MASK (1<<7)
#define OUTPUT3_MASK (POWEREN_MASK | LED_MASK)
#define OUTPUT3_IDLE_MASK (LED_MASK)

#define INPUT_PORT LPC_GPIO3
#define INPUT_BUTTON_MASK (1<<1)
#define INPUT_OUTPUT_ERROR_MASK (1<<2)
#define INPUT_START_MASK (1<<3)
#define INPUT_STAND_MASK (1<<0)

#endif

void hal_board_initialize()
{
	OUTPUT_PORT->DIR |= OUTPUT_MASK;
	OUTPUT_PORT->MASKED_ACCESS[OUTPUT_MASK] = OUTPUT_IDLE_MASK;
	OUTPUT2_PORT->DIR |= OUTPUT2_MASK;
	OUTPUT2_PORT->MASKED_ACCESS[OUTPUT2_MASK] = OUTPUT2_IDLE_MASK;
	OUTPUT3_PORT->DIR |= OUTPUT3_MASK;
	OUTPUT3_PORT->MASKED_ACCESS[OUTPUT3_MASK] = OUTPUT3_IDLE_MASK;

#if defined BOARD_XKUTYCPU1_PROTO || defined BOARD_XKUTYCPU1_EXTENDED
	LPC_IOCON->PIO1_5 = 2 | (2<<3) | (1<<5);	// T32B0.CAP0
	LPC_IOCON->R_PIO1_1 = 3 | (1<<7);	// T32B1.MAT0
	LPC_IOCON->R_PIO1_2 = 1 | (1<<3);	// PIO1_2 
#endif
}

void xcpu_board_output(XCPU_OUTPUT_MASK mask)
{
	OUTPUT_PORT->MASKED_ACCESS[HEADL_MASK] = (mask & OUTPUT_HEADL) ? HEADL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[TAILL_MASK] = (mask & OUTPUT_TAILL) ? TAILL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[BRAKEL_MASK] = (mask & OUTPUT_BRAKEL) ? BRAKEL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[HORN_MASK] = (mask & OUTPUT_HORN) ? HORN_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[AUX_MASK] = (mask & OUTPUT_AUX) ? AUX_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[LEFTL_MASK] = (mask & OUTPUT_LEFTL) ? LEFTL_MASK : 0;
	OUTPUT_PORT->MASKED_ACCESS[RIGHTL_MASK] = (mask & OUTPUT_RIGHTL) ? RIGHTL_MASK : 0;

	OUTPUT2_PORT->MASKED_ACCESS[EBRAKE_MASK] = (mask & OUTPUT_EBRAKE) ? 0 : EBRAKE_MASK;	// active low 
	OUTPUT2_PORT->MASKED_ACCESS[MOTOREN_MASK] = (mask & OUTPUT_MOTOREN) ? MOTOREN_MASK : 0;
	OUTPUT2_PORT->MASKED_ACCESS[HIGHBL_MASK] = (mask & OUTPUT_HIGHBL) ? HIGHBL_MASK : 0;

	OUTPUT3_PORT->MASKED_ACCESS[POWEREN_MASK] = (mask & OUTPUT_POWEREN) ? POWEREN_MASK : 0; 
}

XCPU_INPUT_MASK xcpu_board_input(XCPU_INPUT_MASK mask)
{
	XCPU_INPUT_MASK read = INPUT_NONE;
	unsigned long input = INPUT_PORT->DATA;

	if ((mask & INPUT_STOP_BUT) &&
		!(input & INPUT_BUTTON_MASK)) read |= INPUT_STOP_BUT;

	return read;
}

void xcpu_board_led(int led)
{
	OUTPUT3_PORT->MASKED_ACCESS[LED_MASK] = led ? 0 : LED_MASK;	// led is active low
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

