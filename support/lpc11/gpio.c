#include <support/gpio_hal.h>
#include <CMSIS/LPC11xx.h>

static LPC_GPIO_TypeDef *_gpio[] = { LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3 };
static HAL_GPIO_HANDLER _handlers[4][12];
static const int _irqn[] = { EINT0_IRQn, EINT1_IRQn, EINT2_IRQn, EINT3_IRQn };

int hal_gpio_set_handler(int port, int pin, HAL_GPIO_TRIGGER trigger, HAL_GPIO_HANDLER handler)
{
	if (port < 4 && pin < 12)
	{
		unsigned int mask = 1 << pin;
		if (trigger != HAL_GPIO_INT_NONE)
		{
			_handlers[port][pin] = handler;
	
			_gpio[port]->IS = (_gpio[port]->IS & ~mask) | ((trigger & HAL_GPIO_INT_EDGE) ? 0 : mask);
			_gpio[port]->IBE = (_gpio[port]->IBE & ~mask) | ((trigger == HAL_GPIO_INT_BOTH_EDGES) ? mask : 0);
			_gpio[port]->IEV = (_gpio[port]->IEV & ~mask) | ((trigger & HAL_GPIO_INT_HIGH) ? mask : 0);
			_gpio[port]->IC = mask;
			_gpio[port]->IE |= mask;

			NVIC_EnableIRQ(_irqn[port]);
		}
		else
		{
			_gpio[port]->IE &= ~mask;
		}
		return 1;
	}
	return 0;
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

static void _isr(int port)
{
	LPC_GPIO_TypeDef *gpio = _gpio[port];
	unsigned int mask = gpio->MIS;
	for (int pin = 0; pin < 12; pin++)
	{
		if (mask & (1 << pin))
		{
			HAL_GPIO_HANDLER handler = _handlers[port][pin];
			if (handler)
				handler(port, pin);

			gpio->IC = (1 << pin);
		}
	}
}

void PIOINT0_IRQHandler()
{
	_isr(0);
}

void PIOINT1_IRQHandler()
{
	_isr(1);
}

void PIOINT2_IRQHandler()
{
	_isr(2);
}

void PIOINT3_IRQHandler()
{
	_isr(3);
}

