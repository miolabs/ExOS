#include <support/pwm_hal.h>
#include <CMSIS/nrf51.h>
#include <CMSIS/nrf51_bitfields.h>
#include "timer.h"

static NRF_TIMER_Type *_modules[] = { NRF_TIMER0, NRF_TIMER1, NRF_TIMER2 };
static HAL_PWM_HANDLER _pwm_handlers[3];

static NRF_TIMER_Type *_initialize(int module, unsigned int freq, unsigned int *actual_freq)
{
	if (module > 2)
		return (NRF_TIMER_Type *)0;

	unsigned divider = (SystemCoreClock + (freq >> 1)) / freq;
	unsigned prescaler;
	for(prescaler = 0; prescaler < 9; prescaler++)
	{
		divider >>= 1;
		if (divider == 0) break;
	}
	divider = 1 << prescaler;
	*actual_freq = SystemCoreClock / divider;

	NRF_TIMER_Type *timer = _modules[module];
	timer->TASKS_STOP = 1;
	timer->PRESCALER = prescaler;
	timer->MODE = 0;	// timer mode
	timer->BITMODE = 3;	// 32 bit
	return timer;
}

static void _irq(int module)
{
	NRF_TIMER_Type *timer = _modules[module];
	for(int i = 0; i < 4; i++)
	{
		if (timer->EVENTS_COMPARE[i])
		{
			timer->EVENTS_COMPARE[i] = 0;
			HAL_PWM_HANDLER handler = _pwm_handlers[module];
			if (handler)
				handler(module);
		}
	}
}

void TIMER0_IRQHandler()
{
	_irq(0);
}

void TIMER1_IRQHandler()
{
	_irq(1);
}

void TIMER2_IRQHandler()
{
	_irq(2);
}


int hal_pwm_initialize(int module, int range, int rate, int channel_for_period)
{
	unsigned int actual;
	NRF_TIMER_Type *timer = _initialize(module, range * rate, &actual);
	if (timer)
	{
		switch(channel_for_period)
		{
			case 0: timer->SHORTS = TIMER_COMPARE_CLEAR0;	break;
			case 1: timer->SHORTS = TIMER_COMPARE_CLEAR1;	break;
			case 2: timer->SHORTS = TIMER_COMPARE_CLEAR2;	break;
			case 3: timer->SHORTS = TIMER_COMPARE_CLEAR3;	break;
		}

		unsigned actual_rate = actual / range;
		timer->CC[0] = (channel_for_period == 0) ? range - 1 : range;
		timer->CC[1] = (channel_for_period == 1) ? range - 1 : range;
		timer->CC[2] = (channel_for_period == 2) ? range - 1 : range;
		timer->CC[3] = (channel_for_period == 3) ? range - 1 : range;
		
		timer->TASKS_START = 1;
		return actual_rate;
	}
	return 0; 
}

void hal_pwm_set_handler(int module, int channel, HAL_PWM_HANDLER callback)
{
	_pwm_handlers[module] = callback;
	NRF_TIMER_Type *timer = _modules[module];

	switch(channel)
	{
		case 0:	timer->INTENSET = TIMER_INTEN_COMPARE0; break;
		case 1:	timer->INTENSET = TIMER_INTEN_COMPARE1; break;
		case 2:	timer->INTENSET = TIMER_INTEN_COMPARE2; break;
		case 3:	timer->INTENSET = TIMER_INTEN_COMPARE3; break;
	}

	switch(module)
	{
		case 0: NVIC_EnableIRQ(TIMER0_IRQn);	break;
		case 1: NVIC_EnableIRQ(TIMER1_IRQn);	break;
		case 2: NVIC_EnableIRQ(TIMER2_IRQn);	break;
	}
}

void hal_pwm_set_output(int module, int channel, int value)
{
	NRF_TIMER_Type *timer = _modules[module];
	switch(channel)
	{
		case 0:	timer->CC[0] = value;	break;
		case 1:	timer->CC[1] = value;	break;
		case 2:	timer->CC[2] = value;	break;
		case 3: timer->CC[3] = value; break;
	}
}

void hal_pwm_sync(unsigned mask)
{
	// TODO
}

void hal_pwm_set_period(int module, int value)
{
	// TODO
}