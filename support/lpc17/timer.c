// LPC17xx Timer Peripheral Support
// by Miguel Fides

#include "timer.h"
#include "cpu.h"
#include <support/cap_hal.h>
#include <support/board_hal.h>

static HAL_CAP_HANDLER _capture_handlers[4];
static TIMER_MAT_HANDLER _match_handlers[4];

static LPC_TIM_TypeDef *_initialize(int module, unsigned long freq, TIMER_MAT_HANDLER callback)
{
	LPC_TIM_TypeDef *timer;
    IRQn_Type irq;
	int pclk_div;
	switch(module)
	{
	case 0:
#if (__TARGET_PROCESSOR == LPC1778 || __TARGET_PROCESSOR == LPC1788)
		pclk_div = 1;
#else
		pclk_div = PCLKSEL0bits.PCLK_TIMER0;
#endif
		LPC_SC->PCONP |= PCONP_PCTIM0;
		timer = LPC_TIM0;
		irq = TIMER0_IRQn;
		break;
	case 1:
#if (__TARGET_PROCESSOR == LPC1778 || __TARGET_PROCESSOR == LPC1788)
		pclk_div = 1;
#else	
		pclk_div = PCLKSEL0bits.PCLK_TIMER1;
#endif
		LPC_SC->PCONP |= PCONP_PCTIM1;
		timer = LPC_TIM1;
		irq = TIMER1_IRQn;
		break;
	case 2:
#if (__TARGET_PROCESSOR == LPC1778 || __TARGET_PROCESSOR == LPC1788)
		pclk_div = 1;
#else	
		pclk_div = PCLKSEL1bits.PCLK_TIMER2;
#endif
		LPC_SC->PCONP |= PCONP_PCTIM2;
		timer = LPC_TIM2;
		irq = TIMER2_IRQn;
		break;
	case 3:
#if (__TARGET_PROCESSOR == LPC1778 || __TARGET_PROCESSOR == LPC1788)
		pclk_div = 1;
#else	
		pclk_div = PCLKSEL1bits.PCLK_TIMER3;
#endif
		LPC_SC->PCONP |= PCONP_PCTIM3;
		timer = LPC_TIM3;
		irq = TIMER3_IRQn;
		break;
	default:
		return (LPC_TIM_TypeDef *)0;
	}

	unsigned long pclk = cpu_pclk(SystemCoreClock, pclk_div);
	unsigned long prsc = (pclk + (freq / 2)) / freq;
	timer->TCR = 0;
	timer->PR = prsc != 0 ? prsc - 1 : 0;
	timer->MCR = 0;					// disable matches
	timer->CCR = 0;
	timer->TC = 0;

	_capture_handlers[module] = (HAL_CAP_HANDLER)0;
	_match_handlers[module] = callback;

	NVIC_EnableIRQ(irq);
	return timer;
}

int timer_initialize(int module, unsigned long freq, TIMER_MAT_HANDLER callback)
{
	int mask = hal_board_init_pinmux(HAL_RESOURCE_MAT, module);
	_initialize(module, freq, callback);
	return mask;
}

int hal_cap_initialize(int module, unsigned long freq, HAL_CAP_MODE mode, HAL_CAP_HANDLER callback)
{
	int mask = hal_board_init_pinmux(HAL_RESOURCE_CAP, module);
	LPC_TIM_TypeDef *timer = _initialize(module, freq, 0);
	
	if (timer)
	{
		_capture_handlers[module] = callback;
	
		TIMER_CAPM capm = CAPM_INT;
		if (mode & HAL_CAP_RISING) capm |= CAPM_RISING;
		if (mode & HAL_CAP_FALLING) capm |= CAPM_FALLING;
	
		TIMER_CCR *ccr = (TIMER_CCR *)&timer->CCR;
		ccr->CR0 = capm;
		ccr->CR1 = capm;

		timer->TCR = 1;
		return 1;
	}
	return 0;
}

static void _isr(int module, LPC_TIM_TypeDef *timer)
{
//	TIMER_EVENT_HANDLER match_handler = _match_handlers[module];
	if (timer->IR & TIMER_IR_MR0)
	{
//		if (match_handler) match_handler(0, timer->MR[0], timer->TC);
		timer->IR = TIMER_IR_MR0;
	}
	if (timer->IR & TIMER_IR_MR1)
	{
//		if (match_handler) match_handler(1, timer->MR[1], timer->TC);
		timer->IR = TIMER_IR_MR1;
	}
	if (timer->IR & TIMER_IR_MR2)
	{
//		if (match_handler) match_handler(2, timer->MR[2], timer->TC);
		timer->IR = TIMER_IR_MR2;
	}
	if (timer->IR & TIMER_IR_MR3)
	{
//		if (match_handler) match_handler(3, timer->MR[3], timer->TC);
		timer->IR = TIMER_IR_MR3;
	}
    
	HAL_CAP_HANDLER capture_handler = _capture_handlers[module];
	if (timer->IR & TIMER_IR_CR0)
	{
   		if (capture_handler) capture_handler(0, timer->CR0);
		timer->IR = TIMER_IR_CR0;
	}
	if (timer->IR & TIMER_IR_CR1)
	{
   		if (capture_handler) capture_handler(1, timer->CR1);
		timer->IR = TIMER_IR_CR1;
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

