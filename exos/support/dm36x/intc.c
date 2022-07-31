#include "intc.h"

static INTC_CONTROLLER *_intc = (INTC_CONTROLLER *)0x01C48000;

void intc_set_priority(int number, int enable, INTC_PRI pri)
{
	int reg_index = number >> 3;
	int field_bit = (number & 0x7) << 2;
	unsigned long mask = (0xf << field_bit);

	_intc->INTPRI[reg_index] = (_intc->INTPRI[reg_index] & ~mask) | 
		((pri & 0x7) << field_bit);
	if (enable)
	{
		_intc->EINT |= (1LL << number);
	}
	else
	{
		_intc->EINT &= ~(1LL << number);
	}
}

