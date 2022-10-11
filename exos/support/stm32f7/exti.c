#include "exti.h"
#include "cpu.h"
#include <kernel/panic.h>

static exti_handler_t _handlers[23];

void exti_edge_config(unsigned index, exti_flags_t flags, exti_handler_t handler)
{
	ASSERT(index < 23, KERNEL_ERROR_KERNEL_PANIC);

	unsigned mask = (1 << index);
	if (flags != EXTIF_NONE)
	{
		if (flags & EXTIF_RISING)
			EXTI->RTSR |= mask;
		else
			EXTI->RTSR &= ~mask;

		if (flags & EXTIF_FALLING)
			EXTI->FTSR |= mask;
		else
			EXTI->FTSR &= ~mask;
	
		EXTI->PR = mask;
		EXTI->IMR |= mask;

		switch(index)
		{
			case 0:	NVIC_EnableIRQ(EXTI0_IRQn);	break;
			case 1:	NVIC_EnableIRQ(EXTI1_IRQn);	break;
			case 2:	NVIC_EnableIRQ(EXTI2_IRQn);	break;
			case 3:	NVIC_EnableIRQ(EXTI3_IRQn);	break;
			case 4:	NVIC_EnableIRQ(EXTI4_IRQn);	break;
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
				NVIC_EnableIRQ(EXTI9_5_IRQn);
				break;
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				NVIC_EnableIRQ(EXTI15_10_IRQn);
				break;
		}
		_handlers[index] = handler;
	}
	else
	{
		EXTI->IMR &= ~mask;
		_handlers[index] = nullptr;
	}

}

unsigned exti_pin_config(unsigned pin, exti_flags_t flags, exti_handler_t handler)
{
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	unsigned index = pin & 15;
	exti_port_t port = pin >> 4;

	// NOTE: on stm32f4, first 16 edge detectors are mapped to same-bit gpio ports 
	if (index < 16)
	{
		// configure source gpio port for external EXTI
		unsigned icr_index = index >> 2;
		unsigned shift = (index & 3) << 2;
		unsigned icr_mask = 0xf << shift;
		SYSCFG->EXTICR[icr_index] = (SYSCFG->EXTICR[icr_index] & ~icr_mask) |
			((port << shift) & icr_mask);
	}

	exti_edge_config(index, flags, handler);
	return index;
}


#pragma GCC optimize(2)

static void _irq(unsigned mask, unsigned index)
{
	for(unsigned m2 = mask & (mask ^ (mask << 1)); m2 & mask; m2 <<= 1)
	{
		if (EXTI->PR & m2)
		{
			ASSERT(index < sizeof(_handlers), KERNEL_ERROR_KERNEL_PANIC);
			exti_handler_t handler = _handlers[index];
			if (handler != nullptr)
				handler(index);
		}
		index++;
	}

	EXTI->PR = mask;
}

void EXTI0_IRQHandler()
{
	_irq(1<<0, 0);
}

void EXTI1_IRQHandler()
{
	_irq(1<<1, 1);
}

void EXTI2_IRQHandler()
{
	_irq(1<<2, 2);
}

void EXTI3_IRQHandler()
{
	_irq(1<<3, 3);

}

void EXTI4_IRQHandler()
{
	_irq(1<<4, 4);
}

void EXTI9_5_IRQHandler()
{
	_irq((1<<9) | (1<<8) | (1<<7) | (1<<6) | (1<<5), 5);
}

void EXTI15_10_IRQHandler()
{
	_irq((1<<15) | (1<<14) | (1<<13) | (1<<12) | (1<<11) | (1<<10), 10);
}

