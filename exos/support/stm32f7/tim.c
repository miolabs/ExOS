#include "tim.h"
#include <kernel/panic.h>

static TIM_TypeDef *const _modules[] = { TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12 };
#define TIMER_MODULE_COUNT (sizeof(_modules) / sizeof(void *))
static timer_handler_t _handlers[TIMER_MODULE_COUNT];

static TIM_TypeDef *_initialize(unsigned module, unsigned long freq)
{
	TIM_TypeDef *timer;
    IRQn_Type irq;
	unsigned long pclk;
	switch(module)
	{
		case 0:
			pclk = cpu_get_pclk2();
			RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
			RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
			RCC->APB2RSTR ^= RCC_APB2RSTR_TIM1RST;
			timer = TIM1;
			irq = TIM1_CC_IRQn;
			break;
		case 1:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM2RST;
			timer = TIM2;
			irq = TIM2_IRQn;
			break;
		case 2:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM3RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM3RST;
			timer = TIM3;
			irq = TIM3_IRQn;
			break;
		case 3:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM4RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM4RST;
			timer = TIM4;
			irq = TIM4_IRQn;
			break;
		case 4:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM5RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM5RST;
			timer = TIM5;
			irq = TIM5_IRQn;
			break;
		case 5:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM6RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM6RST;
			timer = TIM6;
			irq = TIM6_DAC_IRQn;
			break;
		case 6:
			pclk = cpu_get_pclk1();
			RCC->APB1RSTR |= RCC_APB1RSTR_TIM7RST;
			RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
			RCC->APB1RSTR ^= RCC_APB1RSTR_TIM7RST;
			timer = TIM7;
			irq = TIM7_IRQn;
			break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
	}
	ASSERT(timer == _modules[module], KERNEL_ERROR_KERNEL_PANIC);

	ASSERT(freq != 0, KERNEL_ERROR_KERNEL_PANIC);
	unsigned long prsc = (pclk + (freq / 2)) / freq;
	timer->CR1 = 0;	// FIXME: set clk division
	timer->PSC = prsc != 0 ? prsc - 1 : 0;
	timer->CCMR1 = timer->CCMR2 = 0;				// disable matches
	timer->CCER = 0;

//	NVIC_EnableIRQ(irq);
	return timer;
}

static void _isr(unsigned module, TIM_TypeDef *timer)
{
	// TODO
}

void TIM1_CC_IRQHandler()
{
	_isr(0, TIM1);
}

void TIM2_IRQHandler()
{
	_isr(1, TIM2);
}

bool hal_pwm_initialize(unsigned module, unsigned range, unsigned rate, unsigned channel_count)
{
	TIM_TypeDef *timer = _initialize(module, range * rate);
	if (timer)
	{
		timer->ARR = range - 1;
		if (channel_count >= 1)
		{
			timer->CCR1 = 0;
			timer->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1P);
			timer->CCMR1 &= ~(TIM_CCMR1_OC1CE | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE | TIM_CCMR1_OC1FE | TIM_CCMR1_CC1S_Msk);
			timer->CCMR1 |= (0 << TIM_CCMR1_CC1S_Pos) // output mode
				| TIM_CCMR1_OC1PE	// Preload Enable (changes access shadow register, update on update event)
				| (TIM_OC_MODE_PWM_MODE1 << TIM_CCMR1_OC1M_Pos);
#ifdef PWM_POLARITY_LOW
			timer->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1P;
#else
			timer->CCER |= TIM_CCER_CC1E;
#endif
		}
		if (channel_count >= 2)
		{
			timer->CCR2 = 0;
			timer->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2P);
			timer->CCMR1 &= ~(TIM_CCMR1_OC2CE | TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2PE | TIM_CCMR1_OC2FE | TIM_CCMR1_CC2S_Msk);
			timer->CCMR1 |= (0 << TIM_CCMR1_CC2S_Pos) // output mode
				| TIM_CCMR1_OC2PE	// Preload Enable (changes access shadow register, update on update event)
				| (TIM_OC_MODE_PWM_MODE1 << TIM_CCMR1_OC2M_Pos);
#ifdef PWM_POLARITY_LOW
			timer->CCER |= TIM_CCER_CC2E | TIM_CCER_CC2P;
#else
			timer->CCER |= TIM_CCER_CC2E;
#endif
		}
		if (channel_count >= 3)
		{
			timer->CCR3 = 0;
			timer->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3P);
			timer->CCMR2 &= ~(TIM_CCMR2_OC3CE | TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE | TIM_CCMR2_OC3FE | TIM_CCMR2_CC3S_Msk);
			timer->CCMR2 |= (0 << TIM_CCMR2_CC3S_Pos) // output mode
				| TIM_CCMR2_OC3PE	// Preload Enable (changes access shadow register, update on update event)
				| (TIM_OC_MODE_PWM_MODE1 << TIM_CCMR2_OC3M_Pos);
#ifdef PWM_POLARITY_LOW
			timer->CCER |= TIM_CCER_CC3E | TIM_CCER_CC3P;
#else
			timer->CCER |= TIM_CCER_CC3E;
#endif
		}
		if (channel_count >= 4)
		{
			timer->CCR4 = 0;
			timer->CCER &= ~(TIM_CCER_CC4E | TIM_CCER_CC4P);
			timer->CCMR2 &= ~(TIM_CCMR2_OC4CE | TIM_CCMR2_OC4M_Msk | TIM_CCMR2_OC4PE | TIM_CCMR2_OC4FE | TIM_CCMR2_CC4S_Msk);
			timer->CCMR2 |= (0 << TIM_CCMR2_CC4S_Pos) // output mode
				| TIM_CCMR2_OC4PE	// Preload Enable (changes access shadow register, update on update event)
				| (TIM_OC_MODE_PWM_MODE1 << TIM_CCMR2_OC4M_Pos);
#ifdef PWM_POLARITY_LOW
			timer->CCER |= TIM_CCER_CC4E | TIM_CCER_CC4P;
#else
			timer->CCER |= TIM_CCER_CC4E;
#endif
		}
		timer_control(module, TIMER_CTRLF_START);
	}
}

void hal_pwm_set_output(unsigned module, unsigned channel, unsigned value)
{
	ASSERT(module < TIMER_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	TIM_TypeDef *timer = _modules[module];
	switch(channel)
	{
		case 0:	timer->CCR1 = value;	break;
		case 1:	timer->CCR2 = value;	break;
		case 2:	timer->CCR3 = value;	break;
		case 3:	timer->CCR4 = value;	break;
	}
}

void hal_pwm_set_period(unsigned module, unsigned value)
{
	// TODO
}

void timer_initialize(unsigned module, unsigned long freq, timer_handler_t handler, timer_ctrl_flags_t flags)
{
	TIM_TypeDef *timer = _initialize(module, freq);
	if (timer)
	{
		_handlers[module] = handler;
		timer_control(module, flags);
	}
}

void timer_match_setup(unsigned module, unsigned channel, unsigned long value, mat_flags_t flags)
{
	ASSERT(module < TIMER_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	TIM_TypeDef *timer = _modules[module];
	//TODO
}

void timer_capture_setup(unsigned module, unsigned channel, cap_flags_t flags)
{
	ASSERT(module < TIMER_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	TIM_TypeDef *timer = _modules[module];
	switch (channel)
	{
		case 0:
			timer->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NE | TIM_CCER_CC1NP);
			timer->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1F_Msk | TIM_CCMR1_IC1PSC_Msk);
			// TODO
			timer->CCER |= TIM_CCER_CC1E;
			break;
	}
}

void timer_control(unsigned module, timer_ctrl_flags_t flags)
{
	ASSERT(module < TIMER_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	TIM_TypeDef *timer = _modules[module];

	if (flags & TIMER_CTRLF_START)
	{
		if (flags & TIMER_CTRLF_RESET)
			timer->CNT = 0;
		timer->CR1 = TIM_CR1_CEN;
	}
	else if (flags & TIMER_CTRLF_STOP)
	{
		timer->CR1 &= ~TIM_CR1_CEN;
		if (flags & TIMER_CTRLF_RESET)
			timer->CNT = 0;
	}
}

unsigned long timer_value(unsigned module)
{
	ASSERT(module < TIMER_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	TIM_TypeDef *timer = _modules[module];
	return timer->CNT;
}

