#include "gpio.h"
#include "cpu.h"
#include <kernel/panic.h>

static LPC_GPIO_TypeDef *_gpio[] = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3, LPC_GPIO4 };

static const unsigned _port_count = (sizeof(_gpio) / sizeof(LPC_GPIO_TypeDef *));

//void hal_gpio_write(int port, unsigned int mask, unsigned int state)
//{
//	_gpio[port]->FIOSET = mask & state;
//	_gpio[port]->FIOCLR = mask & ~state;
//}

//unsigned int hal_gpio_read(int port, unsigned int mask)
//{
//	return _gpio[port]->FIOPIN & mask;
//}

void hal_gpio_pin_set(unsigned pin, bool state)
{
	unsigned port = (pin >> 5);
	ASSERT(port < _port_count, KERNEL_ERROR_KERNEL_PANIC);

	unsigned mask = 1 << (pin & 31);
	if (state) _gpio[port]->SET = mask;
	else _gpio[port]->CLR = mask;
}

bool hal_gpio_pin(unsigned pin)
{
	unsigned port = (pin >> 5);
	ASSERT(port < _port_count, KERNEL_ERROR_KERNEL_PANIC);

	unsigned mask = 1 << (pin & 31);
	return (_gpio[port]->PIN & mask) != 0;
}

void hal_gpio_pin_config(unsigned pin, hal_gpio_flags_t flags)
{
	unsigned port = (pin >> 5);
	ASSERT(port < _port_count, KERNEL_ERROR_KERNEL_PANIC);

	unsigned mask = 1 << (pin & 31);
	_gpio[port]->DIR = (_gpio[port]->DIR & ~mask) | ((flags & GPIOF_OUTPUT) ? mask : 0);

	// TODO
}