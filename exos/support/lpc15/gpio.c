// GPIO module support for LPC15xx
// by Miguel Fides

#include <support/gpio_hal.h>
#include <CMSIS/LPC15xx.h>

#pragma GCC optimize(2)

//int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler);

void hal_gpio_write(int port, unsigned int mask, unsigned int state)
{
	// TODO
}

unsigned int hal_gpio_read(int port, unsigned int mask)
{
	// TODO
}

void hal_gpio_pin_set(int port, int pin, int state)
{
	LPC_GPIO_PORT->B[(port << 5) + (pin & 31)] = state ? 1 : 0;
}

int hal_gpio_pin(int port, int pin)
{
	return LPC_GPIO_PORT->B[(port << 5) + (pin & 31)];
}

void hal_gpio_config(int port, unsigned int mask, unsigned int output)
{
	unsigned int dir = LPC_GPIO_PORT->DIR[port];
	LPC_GPIO_PORT->DIR[port] = (dir & ~mask) | (mask & output);
}


