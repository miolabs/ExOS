#include "dac.h"
#include "cpu.h"
#include <stdbool.h>

void dac_initialize(unsigned ch, stm32_dac_trigger_t trig, stm32_dac_flags_t flags, unsigned short initial_value)
{
	static bool _init_done = false;

	if (!_init_done)
	{
		_init_done = true;
		RCC->APB1RSTR |= RCC_APB1RSTR_DACRST;
		RCC->APB1ENR |= RCC_APB1ENR_DACEN;
		RCC->APB1RSTR ^= RCC_APB1RSTR_DACRST;
	}

	if (flags & STM32_DACF_INITIAL)
	{
		// write initial value to avoid glitch
		dac_write16(ch, initial_value);
	}

	unsigned short cfg = DAC_CR_EN1
		| ((flags & STM32_DACF_TEN) ? DAC_CR_TEN1 : 0)
		| ((flags & STM32_DACF_BUF_OFF) ? DAC_CR_BOFF1 : 0)
		| (trig << DAC_CR_TSEL1_Pos)
		| ((flags & STM32_DACF_DMA) ? DAC_CR_DMAEN1 : 0)
		| ((flags & STM32_DACF_UDRIE) ? DAC_CR_DMAUDRIE1 : 0);

	switch(ch)
	{
		case 0:	DAC->CR = (DAC->CR & 0xFFFF0000UL) | (cfg << 0);	break;
		case 1:	DAC->CR = (DAC->CR & 0x0000FFFFUL) | (cfg << 16);	break;
	}
}

void dac_write16(unsigned ch, unsigned short value)
{
	switch(ch)
	{
		case 0:	DAC->DHR12L1 = value;	break;
		case 1:	DAC->DHR12L2 = value;	break;
	}
}




