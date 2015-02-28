// LPC17xx Timer Peripheral Support
// by Miguel Fides

#include "timer.h"
#include "cpu.h"
#include <support/cap_hal.h>
#include <support/pwm_hal.h>

static LPC_TIM_TypeDef *_modules[] = { LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3 };
static HAL_CAP_HANDLER _cap_handlers[4];
static HAL_PWM_HANDLER _mat_handlers[4];

static LPC_TIM_TypeDef *_initialize(int module, unsigned long freq)
{
	LPC_TIM_TypeDef *timer;
    IRQn_Type irq;
	unsigned long pclk;
	switch(module)
	{
	case 0:
		pclk = cpu_pclk(PCLK_TIMER0);
		LPC_SC->PCONP |= PCONP_PCTIM0;
		timer = LPC_TIM0;
		irq = TIMER0_IRQn;
		break;
	case 1:
		pclk = cpu_pclk(PCLK_TIMER1);
		LPC_SC->PCONP |= PCONP_PCTIM1;
		timer = LPC_TIM1;
		irq = TIMER1_IRQn;
		break;
	case 2:
		pclk = cpu_pclk(PCLK_TIMER2);
		LPC_SC->PCONP |= PCONP_PCTIM2;
		timer = LPC_TIM2;
		irq = TIMER2_IRQn;
		break;
	case 3:
		pclk = cpu_pclk(PCLK_TIMER3);
		LPC_SC->PCONP |= PCONP_PCTIM3;
		timer = LPC_TIM3;
		irq = TIMER3_IRQn;
		break;
	default:
		return (LPC_TIM_TypeDef *)0;
	}

	unsigned long prsc = (pclk + (freq / 2)) / freq;
	timer->TCR = 0;
	timer->PR = prsc != 0 ? prsc - 1 : 0;
	timer->MCR = 0;					// disable matches
	timer->CCR = 0;
	timer->TC = 0;

//	_capture_handlers[module] = (HAL_CAP_HANDLER)0;
//	_match_handlers[module] = callback;
//
//	NVIC_EnableIRQ(irq);
	return timer;
}

int hal_cap_initialize(int module, unsigned long freq, HAL_CAP_MODE mode, HAL_CAP_HANDLER callback)
{
	LPC_TIM_TypeDef *timer = _initialize(module, freq);
	if (timer)
	{
		_cap_handlers[module] = callback;
		timer->CCR = (callback ? TIMER_CCR_CAP0I : 0) |
			((mode & HAL_CAP_RISING) ? TIMER_CCR_CAP0RE : 0) |
            ((mode & HAL_CAP_FALLING) ? TIMER_CCR_CAP0FE : 0);

		switch(module)
		{
			case 0: NVIC_EnableIRQ(TIMER0_IRQn);	break;
			case 1: NVIC_EnableIRQ(TIMER1_IRQn);	break;
			case 2: NVIC_EnableIRQ(TIMER2_IRQn);	break;
			case 3: NVIC_EnableIRQ(TIMER3_IRQn);	break;
		}

		timer->TCR = 1;
		return 1;
	}
	return 0;
}

static void _isr(int module, LPC_TIM_TypeDef *timer)
{
	if (timer->IR & (TIMER_IR_CAP0 | TIMER_IR_CAP1))
	{
		HAL_CAP_HANDLER handler = _cap_handlers[module];
		if (handler)
		{
			if (timer->IR & TIMER_IR_CAP0)
			{
				handler(0, timer->CR0);
				timer->IR = TIMER_IR_CAP0;
			}
			if (timer->IR & TIMER_IR_CAP1)
			{
				handler(1, timer->CR1);
				timer->IR = TIMER_IR_CAP1;
			}
		}
	}

	if (timer->IR & (TIMER_IR_MAT0|TIMER_IR_MAT1|TIMER_IR_MAT2|TIMER_IR_MAT3))
	{
		HAL_PWM_HANDLER handler = _mat_handlers[module];
		if (handler)
			handler(module);
		// NOTE: We are only interested in one match channel irq for pwm
		timer->IR = (TIMER_IR_MAT0|TIMER_IR_MAT1|TIMER_IR_MAT2|TIMER_IR_MAT3);
	}
}

void TIMER0_IRQHandler()
{
	_isr(0, LPC_TIM0);
}

void TIMER1_IRQHandler()
{
	_isr(1, LPC_TIM1);
}

void TIMER2_IRQHandler()
{
	_isr(2, LPC_TIM2);
}

void TIMER3_IRQHandler()
{
	_isr(3, LPC_TIM3);
}

int timer_match_initialize(int module, int range, int rate, int channel_for_period)
{
	LPC_TIM_TypeDef *timer = _initialize(module, range * rate);
	if (timer)
	{
		timer->MR0 = (channel_for_period == 0) ? range - 1 : range;
		timer->MR1 = (channel_for_period == 1) ? range - 1 : range;
		timer->MR2 = (channel_for_period == 2) ? range - 1 : range;
		timer->MR3 = (channel_for_period == 3) ? range - 1 : range;

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

void timer_match_set_handler(int module, int channel, HAL_PWM_HANDLER callback)
{
       	_mat_handlers[module] = callback;
		LPC_TIM_TypeDef *timer = _modules[module];

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
				NVIC_EnableIRQ(TIMER0_IRQn);	
				NVIC_SetPriority(TIMER0_IRQn, 0);
				break;
			case 1: NVIC_EnableIRQ(TIMER1_IRQn);	break;
			case 2: NVIC_EnableIRQ(TIMER2_IRQn);	break;
			case 3: NVIC_EnableIRQ(TIMER3_IRQn);	break;
		}
}

void timer_match_set_value(int module, int channel, int value)
{
	LPC_TIM_TypeDef *timer = _modules[module];
	switch(channel)
	{
		case 0:	timer->MR0 = value;	break;
		case 1:	timer->MR1 = value;	break;
		case 2:	timer->MR2 = value;	break;
		case 3: timer->MR3 = value; break;
	}
}

void timer_match_set_trigger(int module, int channel, int trigger, int state)
{
	LPC_TIM_TypeDef *timer = _modules[module];
	switch(channel)
	{
		case 0:
			timer->EMR = (timer->EMR & ~(TIMER_EMR_EM0 | TIMER_EMR_EMC0_MASK)) |
				(trigger << TIMER_EMR_EMC0_BIT) | 
				(state ? TIMER_EMR_EM0 : 0);
			break;
		case 1:
			timer->EMR = (timer->EMR & ~(TIMER_EMR_EM1 | TIMER_EMR_EMC1_MASK)) |
				(trigger << TIMER_EMR_EMC1_BIT) | 
				(state ? TIMER_EMR_EM1 : 0);
			break;
		case 2:
			timer->EMR = (timer->EMR & ~(TIMER_EMR_EM2 | TIMER_EMR_EMC2_MASK)) |
				(trigger << TIMER_EMR_EMC2_BIT) | 
				(state ? TIMER_EMR_EM2 : 0);
			break;
		case 3:
			timer->EMR = (timer->EMR & ~(TIMER_EMR_EM3 | TIMER_EMR_EMC3_MASK)) |
				(trigger << TIMER_EMR_EMC3_BIT) | 
				(state ? TIMER_EMR_EM3 : 0);
			break;
	}	
}

unsigned long timer_actual(int module)
{
	LPC_TIM_TypeDef *timer = _modules[module];
	return timer->TC;
}
