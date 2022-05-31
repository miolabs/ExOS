#include "gpio.h"
#include <support/gpio_hal.h>
#include <NUC1xx.h>

static GPIO_PORT *_gpio[] = { (GPIO_PORT *)GPIOA, (GPIO_PORT *)GPIOB, 
	(GPIO_PORT *)GPIOB, (GPIO_PORT *)GPIOC, (GPIO_PORT *)GPIOD };
static GPIO_PORT_PINS *_gpio_pins[] = { (GPIO_PORT_PINS *)(GPIO_BASE + 0x200), (GPIO_PORT_PINS *)(GPIO_BASE + 0x240), 
	(GPIO_PORT_PINS *)(GPIO_BASE + 0x280), (GPIO_PORT_PINS *)(GPIO_BASE + 0x2C0), (GPIO_PORT_PINS *)(GPIO_BASE + 0x300) };

//static const int _irqn[] = { GPAB_IRQn, GPAB_IRQn, GPCDE_IRQn, GPCDE_IRQn, GPCDE_IRQn };

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler)
{
	// TODO
	return 0;
}

void hal_gpio_write(int port, unsigned int mask, unsigned int state)
{
	_gpio[port]->DOUT = (_gpio[port]->DOUT & mask) | (state & mask);
}

unsigned int hal_gpio_read(int port, unsigned int mask)
{
	return _gpio[port]->PIN & mask;
}

void hal_gpio_pin_set(int port, int pin, int state)
{
	_gpio_pins[port]->PIN[pin] = state;
}

int hal_gpio_pin(int port, int pin)
{
	return _gpio_pins[port]->PIN[pin];
}

void hal_gpio_config(int port, unsigned int mask, unsigned int output)
{
	GPIO_PORT *p = _gpio[port];
	if (mask & (1<<0)) p->PMD0 = (output & 1<<0) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<1)) p->PMD1 = (output & 1<<1) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<2)) p->PMD0 = (output & 1<<2) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<3)) p->PMD1 = (output & 1<<3) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<4)) p->PMD4 = (output & 1<<4) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<5)) p->PMD5 = (output & 1<<5) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<6)) p->PMD6 = (output & 1<<6) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<7)) p->PMD7 = (output & 1<<7) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<8)) p->PMD8 = (output & 1<<8) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<9)) p->PMD9 = (output & 1<<9) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<10)) p->PMD10 = (output & 1<<10) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<11)) p->PMD11 = (output & 1<<11) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<12)) p->PMD12 = (output & 1<<12) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<13)) p->PMD13 = (output & 1<<13) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<14)) p->PMD14 = (output & 1<<14) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
	if (mask & (1<<15)) p->PMD15 = (output & 1<<15) ? GPIO_PMD_OUTPUT : GPIO_PMD_INPUT;
}



