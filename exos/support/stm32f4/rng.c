#include "rng.h"
#include "cpu.h"
#include <support/random_hal.h>
#include <kernel/panic.h>

static bool _init_done = false;

void rng_initialize()
{
	RCC->AHB2RSTR |= RCC_AHB2RSTR_RNGRST;
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
	RCC->AHB2RSTR &= ~RCC_AHB2RSTR_RNGRST;

	RNG->CR = RNG_CR_RNGEN;

	_init_done = true;
}

int hal_random_hw(hal_random_context_t *context, unsigned char *buffer, unsigned length)
{
	ASSERT(context != NULL && buffer != NULL, KERNEL_ERROR_NULL_POINTER);
	if (!_init_done)
	{
		rng_initialize();
	}
	
	unsigned wait = 0;
	unsigned data;
	while(1)
	{
		unsigned sr = RNG->SR;
		ASSERT(0 == (sr & (RNG_SR_SECS | RNG_SR_CECS)), KERNEL_ERROR_KERNEL_PANIC);
		if (sr & RNG_SR_DRDY)
		{
			data = RNG->DR;
			break;
		}
		wait++;
		ASSERT(wait < 100, KERNEL_ERROR_UNKNOWN);
	}
	
	unsigned done = length;
	if (length >= 4)	{ *buffer++ = data >> 24; done = 4; }
	if (length >= 3)	*buffer++ = data >> 16;
	if (length >= 2)	*buffer++ = data >> 8;
	if (length >= 1)	*buffer++ = data;
	return done;
}




