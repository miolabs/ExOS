#ifndef HAL_RANDOM_HAL_H
#define HAL_RANDOM_HAL_H

typedef struct
{
    unsigned int Seed;
} hal_random_context_t;


int hal_random_hw(hal_random_context_t *context, unsigned char *buffer, unsigned length);


static void hal_random_context_create(hal_random_context_t *context, unsigned seed)
{
	*context = (hal_random_context_t) { .Seed = seed };
}

static int hal_random_read(hal_random_context_t *context, unsigned char *buffer, unsigned length)
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


#endif // HAL_RANDOM_HAL_H


