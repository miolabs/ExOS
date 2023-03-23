#include "random_hal.h"
#include <kernel/panic.h>

void hal_random_context_create(hal_random_context_t *context, unsigned seed)
{
	*context = (hal_random_context_t) { .Seed = seed };
}

int hal_random_read(hal_random_context_t *context, unsigned char *buffer, unsigned length)
{
	int done = 0;
	while(done < length)
	{
		int chunk = hal_random_hw(context, buffer + done, length - done);
		if (chunk == 0)
			return -1;
		
		done += chunk;
		length -= chunk;
	}
	return done;
}


