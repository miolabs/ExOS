#include <support/gpio_hal.h>
#include <CMSIS/LPC11xx.h>

static LPC_GPIO_TypeDef *_gpio[] = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler)
{
	// TODO
}

void hal_gpio_write(int port, unsigned int mask, unsigned int state)
{
	_gpio[port]->MASKED_ACCESS[mask & 0xFFF] = state;
}

unsigned int hal_gpio_read(int port, unsigned int mask)
{
	return _gpio[port]->MASKED_ACCESS[mask & 0xFFF];
}

void hal_gpio_config(int port, unsigned int mask, unsigned int output)
{
	mask &= 0xFFF;
	_gpio[port]->DIR = (_gpio[port]->DIR & ~mask) | (output & mask);
}



