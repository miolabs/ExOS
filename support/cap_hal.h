#ifndef HAL_CAP_HAL_H
#define HAL_CAP_HAL_H

typedef void (*HAL_CAP_HANDLER)(int channel, unsigned long time);

typedef enum
{
	HAL_CAP_NONE = 0,
	HAL_CAP_RISING = 1,
	HAL_CAP_FALLING = 2,
	HAL_CAP_BOTH = (HAL_CAP_RISING | HAL_CAP_FALLING),
} HAL_CAP_MODE;

// prototypes
int hal_cap_initialize(int module, int freq, HAL_CAP_MODE mode, HAL_CAP_HANDLER callback);

#endif // HAL_CAP_HAL_H
