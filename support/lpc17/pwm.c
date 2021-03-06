// LPC17xx PWM Peripheral Support
// by Miguel Fides

#include "pwm.h"
#include "cpu.h"
#include <support/pwm_hal.h>
#include <support/board_hal.h>

#define PWM_MODULE_COUNT 1
static LPC_PWM_TypeDef *_modules[PWM_MODULE_COUNT] = { (LPC_PWM_TypeDef *)LPC_PWM1 };

static void _isr_pwm(int module);
static void _setup(int module, unsigned long channel_mask, int period);

int pwm_initialize(int module, int range, int rate, int mask)
{
	int freq = range * rate;
	LPC_PWM_TypeDef *pwm;
	switch(module)
	{
		case 0:
			LPC_SC->PCONP |= PCONP_PCPWM1;
			pwm = LPC_PWM1;
			break;
		default:
			return 0;
	}
	int pclk = cpu_pclk(PCLK_PWM1);
	pwm->TCR = 0;
	pwm->PR = (pclk / freq) - 1;	// preescaler for selected freq
	pwm->MCR = 0;					// disable matches
	pwm->TC = 0;

	NVIC_EnableIRQ(PWM1_IRQn);

	if (mask != 0)
	{
		for (int channel = 0; channel < 6; channel++)
		{
			if (mask & (1<<channel))
				switch(channel)
				{
					case 0: 
						pwm->PCR |= PWM_PCR_PWMENA1;
						break;
					case 1:
						pwm->PCR &= ~PWM_PCR_PWMSEL2;
						pwm->PCR |= PWM_PCR_PWMENA2;
						break;
					case 2:
						pwm->PCR &= ~PWM_PCR_PWMSEL3;
						pwm->PCR |= PWM_PCR_PWMENA3;
						break;
					case 3:
						pwm->PCR &= ~PWM_PCR_PWMSEL4;
						pwm->PCR |= PWM_PCR_PWMENA4;
						break;
					case 4:
						pwm->PCR &= ~PWM_PCR_PWMSEL5;
						pwm->PCR |= PWM_PCR_PWMENA5;
						break;
					case 5:
						pwm->PCR &= ~PWM_PCR_PWMSEL6;
						pwm->PCR |= PWM_PCR_PWMENA6;
						break;
				}
		}
		hal_pwm_set_period(module, range);
		pwm->TCR |= PWM_TCR_PWMEN | PWM_TCR_RUN;
	}
	return 1;
}

int hal_pwm_initialize(int module, int range, int rate, int channel_for_period)
{
	// NOTE: channel_for_period is ignored because module uses separate channel for period
	pwm_initialize(module, range, rate, 0x3f);
}

static void _isr_pwm(int module)
{
	LPC_PWM_TypeDef *pwm = _modules[module];
	
	int match_mask = pwm->IR & PWM_IR_MR_MASK;
	if (match_mask != 0)
	{
		pwm->IR = match_mask; // clear IR bits
	}

	if (pwm->IR & PWM_IR_CR0)
	{
//		if (capture_handler) capture_handler(0, pwm->CR0, pwm->TC);
		pwm->IR = PWM_IR_CR0; // clear IR bit
	}
	if (pwm->IR & PWM_IR_CR1)
	{
//		if (capture_handler) capture_handler(1, pwm->CR0, pwm->TC);
		pwm->IR = PWM_IR_CR1; // clear IR bit
	}
}

void PWM1_IRQHandler()
{
	_isr_pwm(0);
}

void hal_pwm_set_period(int module, int value)
{
	if (module < PWM_MODULE_COUNT)
	{
		LPC_PWM_TypeDef *pwm = _modules[module];
		pwm->MR0 = value;
		pwm->LER |= 1;
	}
}

void hal_pwm_set_output(int module, int channel, int value)
{
	if (module < PWM_MODULE_COUNT)
	{
		LPC_PWM_TypeDef *pwm = _modules[module];
		switch(channel)
		{
			case 0: pwm->MR1 = value; break;
			case 1: pwm->MR2 = value; break;
			case 2: pwm->MR3 = value; break;
			case 3: pwm->MR4 = value; break;
			case 4: pwm->MR5 = value; break;
			case 5: pwm->MR6 = value; break;
		}
		pwm->LER |= (2 << channel);
	}
}


