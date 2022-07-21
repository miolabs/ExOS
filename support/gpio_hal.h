#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <kernel/types.h>
#include <stdbool.h>

typedef enum
{
	GPIOF_INPUT = 0,
	GPIOF_OUTPUT = 1<<0,
	GPIOF_ANALOG = 1<<1,
	GPIOF_PULLUP = 1<<2,
	GPIOF_PULLDOWN = 1<<3,
	GPIOF_OPEN_DRAIN = 1<<4,
} hal_gpio_flags_t;

void hal_gpio_pin_set(unsigned pin, bool state);
bool hal_gpio_pin(unsigned pin);
void hal_gpio_pin_config(unsigned pin, hal_gpio_flags_t flags);

#ifdef EXOS_OLD
typedef enum
{
	HAL_GPIO_INT_NONE = 0,
	HAL_GPIO_INT_LOW = 1,
	HAL_GPIO_INT_HIGH = 2,
	HAL_GPIO_INT_EDGE = 4,
	HAL_GPIO_INT_RISING_EDGE = (HAL_GPIO_INT_HIGH | HAL_GPIO_INT_EDGE),
	HAL_GPIO_INT_FALLING_EDGE = (HAL_GPIO_INT_LOW | HAL_GPIO_INT_EDGE),
	HAL_GPIO_INT_BOTH_EDGES = (HAL_GPIO_INT_RISING_EDGE | HAL_GPIO_INT_FALLING_EDGE),
} HAL_GPIO_TRIGGER;

typedef void (* HAL_GPIO_HANDLER)(int port, int pin); 

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler) __deprecated;
void hal_gpio_config(int port, unsigned int mask, unsigned int output) __deprecated;
void hal_gpio_write(int port, unsigned int mask, unsigned int state) __deprecated;
unsigned int hal_gpio_read(int port, unsigned int mask) __deprecated;
#endif

#endif // HAL_GPIO_H
