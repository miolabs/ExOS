#include <support/pwm_hal.h>
#include <support/cap_hal.h>
#include "timer.h"

// NOTE: Note that timers 2 and 3 are 16 bit timers!
static LPC_TMR_TypeDef *_modules[] = { LPC_TMR32B0, LPC_TMR32B1, LPC_TMR16B0, LPC_TMR16B1 };
static HAL_CAP_HANDLER _cap_handlers[4];
static HAL_PWM_HANDLER _pwm_handlers[4];

static LPC_TMR_TypeDef *_initialize(int module, int freq)
{
	switch(module)
	{
		case 0:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT32B0;
			break;
		case 1:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT32B1;
			break;
		case 2:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT16B0;
			break;
		case 3:
			LPC_SYSCON->SYSAHBCLKCTRL |= SYSAHBCLKCTRL_CT16B1;
			break;
		default:
			return (LPC_TMR_TypeDef *)0;
	}

	LPC_TMR_TypeDef *timer = _modules[module];

	timer->TCR = 0;
	timer->PR = (SystemCoreClock / freq) - 1;
	timer->CTCR = 0;
	timer->PWMC = 0;
    timer->MCR = 0;

	return timer;
}

int hal_cap_initialize(int module, unsigned long freq, HAL_CAP_MODE mode, HAL_CAP_HANDLER callback)
{
	LPC_TMR_TypeDef *timer = _initialize(module, freq);
	if (timer)
	{
		_cap_handlers[module] = callback;
		timer->CCR = (callback ? TIMER_CCR_CAP0I : 0) |
			((mode & HAL_CAP_RISING) ? TIMER_CCR_CAP0RE : 0) |
            ((mode & HAL_CAP_FALLING) ? TIMER_CCR_CAP0FE : 0);

		switch(module)
		{
			case 0: NVIC_EnableIRQ(TIMER_32_0_IRQn);	break;
			case 1: NVIC_EnableIRQ(TIMER_32_1_IRQn);	break;
			case 2: NVIC_EnableIRQ(TIMER_16_0_IRQn);	break;
			case 3: NVIC_EnableIRQ(TIMER_16_1_IRQn);	break;
		}

		timer->TCR = 1;
		return 1;
	}
	return 0;
}

static void _irq(int module)
{
	LPC_TMR_TypeDef *timer = _modules[module];
	if (timer->IR & TIMER_IR_CAP0)
	{
		HAL_CAP_HANDLER handler = _cap_handlers[module];
		if (handler)
			handler(0, timer->CR0);

		timer->IR = TIMER_IR_CAP0;
	}

	if (timer->IR & (TIMER_IR_MAT0|TIMER_IR_MAT1|TIMER_IR_MAT2|TIMER_IR_MAT3))
	{
		HAL_PWM_HANDLER handler = _pwm_handlers[module];
		if (handler)
			handler(module);
		// NOTE: We are only interested in one match channel irq for pwm
		timer->IR = (TIMER_IR_MAT0|TIMER_IR_MAT1|TIMER_IR_MAT2|TIMER_IR_MAT3);
	}
}

void TIMER32_0_IRQHandler()
{
	_irq(0);
}

void TIMER32_1_IRQHandler()
{
	_irq(1);
}

void TIMER16_0_IRQHandler()
{
	_irq(2);
}

void TIMER16_1_IRQHandler()
{
	_irq(3);
}

int hal_pwm_initialize(int module, int range, int rate, int channel_for_period)
{
	LPC_TMR_TypeDef *timer = _initialize(module, range * rate);
	if (timer)
	{
		timer->MR0 = (channel_for_period == 0) ? range - 1 : range;
		timer->MR1 = (channel_for_period == 1) ? range - 1 : range;
		timer->MR2 = (channel_for_period == 2) ? range - 1 : range;
		timer->MR3 = (channel_for_period == 3) ? range - 1 : range;
		timer->PWMC = 0xF ^ (1 << channel_for_period);

		switch(channel_for_period)
		{
			case 0:
				timer->MCR = TIMER_MCR_MR0R;
				break;
			case 1:
				timer->MCR = TIMER_MCR_MR1R;
				break;
			case 2:
				timer->MCR = TIMER_MCR_MR2R;
				break;
			case 3:
				timer->MCR = TIMER_MCR_MR3R;
				break;
		}
		timer->TCR = 1;
		return 1;
	}
	return 0; 
}

void hal_pwm_set_handler(int module, int channel, HAL_PWM_HANDLER callback)
{
       	_pwm_handlers[module] = callback;
		LPC_TMR_TypeDef *timer = _modules[module];

		timer->MCR &= ~(TIMER_MCR_MR0I | TIMER_MCR_MR1I | TIMER_MCR_MR2I | TIMER_MCR_MR3I);
		switch(channel)
		{
			case 0:
				timer->MCR |= TIMER_MCR_MR0I;
				break;
			case 1:
				timer->MCR |= TIMER_MCR_MR1I;
				break;
			case 2:
				timer->MCR |= TIMER_MCR_MR2I;
				break;
			case 3:
				timer->MCR |= TIMER_MCR_MR3I;
				break;
		}

		switch(module)
		{
			case 0: 
				NVIC_EnableIRQ(TIMER_32_0_IRQn);	
				NVIC_SetPriority(TIMER_32_0_IRQn, 0);
				break;
			case 1: NVIC_EnableIRQ(TIMER_32_1_IRQn);	break;
			case 2: NVIC_EnableIRQ(TIMER_16_0_IRQn);	break;
			case 3: NVIC_EnableIRQ(TIMER_16_1_IRQn);	break;
		}
}

void hal_pwm_set_output(int module, int channel, int value)
{
	LPC_TMR_TypeDef *timer = _modules[module];
	switch(channel)
	{
		case 0:	timer->MR0 = value;	break;
		case 1:	timer->MR1 = value;	break;
		case 2:	timer->MR2 = value;	break;
		case 3: timer->MR3 = value; break;
	}
}

void hal_pwm_sync(unsigned mask)
{
	// NOTE: room for improvement...
	if (mask & (1<<0)) _modules[0]->TCR = 2;
	if (mask & (1<<1)) _modules[1]->TCR = 2;
	if (mask & (1<<2)) _modules[2]->TCR = 2;
	if (mask & (1<<3)) _modules[3]->TCR = 2;

	if (mask & (1<<0)) _modules[0]->TCR = 1;
	if (mask & (1<<1)) _modules[1]->TCR = 1;
	if (mask & (1<<2)) _modules[2]->TCR = 1;
	if (mask & (1<<3)) _modules[3]->TCR = 1;
}

void hal_pwm_set_period(int module, int value)
{
	// TODO
}
