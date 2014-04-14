#ifndef HAL_GPIO_H
#define HAL_GPIO_H

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

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler);
void hal_gpio_write(int port, unsigned int mask, unsigned int state);
unsigned int hal_gpio_read(int port, unsigned int mask);
void hal_gpio_pin_set(int port, int pin, int state);
int hal_gpio_pin(int port, int pin); 
void hal_gpio_config(int port, unsigned int mask, unsigned int output);

#endif // HAL_GPIO_H
