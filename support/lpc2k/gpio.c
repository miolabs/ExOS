#include <support/gpio_hal.h>
#include "cpu.h"
#include "pincon.h"

static LPC_GPIO_TypeDef *_gpio[] = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4 };

void hal_gpio_write(int port, unsigned int mask, unsigned int state)
{
	_gpio[port]->FIOSET = mask & state;
	_gpio[port]->FIOCLR = mask & ~state;
}

unsigned int hal_gpio_read(int port, unsigned int mask)
{
	return _gpio[port]->FIOPIN & mask;
}

void hal_gpio_pin_set(int port, int pin, int state)
{
	if (state) _gpio[port]->FIOSET = 1<<pin;
	else _gpio[port]->FIOCLR = 1<<pin;
}

int hal_gpio_pin(int port, int pin)
{
	return hal_gpio_read(port, 1<<pin);
}

void hal_gpio_config(int port, unsigned int mask, unsigned int output)
{
	_gpio[port]->FIODIR = (_gpio[port]->FIODIR & ~mask) | (output & mask);
}