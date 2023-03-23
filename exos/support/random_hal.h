#ifndef HAL_RANDOM_HAL_H
#define HAL_RANDOM_HAL_H

typedef struct
{
    unsigned int Seed;
} hal_random_context_t;

void hal_random_context_create(hal_random_context_t *context, unsigned seed);
int hal_random_read(hal_random_context_t *context, unsigned char *buffer, unsigned length);

int hal_random_hw(hal_random_context_t *context, unsigned char *buffer, unsigned length);

#endif // HAL_RANDOM_HAL_H


