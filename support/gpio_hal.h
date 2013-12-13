#ifndef HAL_GPIO_H
#define HAL_GPIO_H

typedef enum
{
	HAL_INT_LOW_LEVEL = 0,
	HAL_INT_HIGH_LEVEL,
	HAL_INT_FALLING_EDGE,
	HAL_INT_RAISING_EDGE,
} HAL_GPIO_TRIGGER;

typedef void (* HAL_GPIO_HANDLER)(int pin); 

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler);
void hal_gpio_write(int port, unsigned int mask, unsigned int state);
unsigned int hal_gpio_read(int port, unsigned int mask);
void hal_gpio_config(int port, unsigned int mask, unsigned int output);

#endif // HAL_GPIO_H
