#include "gpio.h"
#include "system.h"

static GPIO_MODULE *_gpio = (GPIO_MODULE *)0x01C67000;

void gpio_initialize()
{
	//The input clock to the GPIO peripheral is driven by PLLC1 (SYSCLK4)
	//The maximum operation speed for the GPIO peripheral is 10 MHz
	//pllc1_set_divider(4, 4);	// will make SYSCLK4 = 24 / 4 = 6 MHz

	// De-assert rest for gpio module
    psc_set_module_state(PSC_MODULE_GPIO, PSC_MODULE_ENABLE);
}

static GPIO_PORT *_gio(int gio, unsigned long *pmask)
{
	if (gio < 32)
	{
		*pmask = (1<<gio);
		return &_gpio->Port01;
	}
	else if (gio < 64)
	{
		*pmask = (1<<(gio - 32));
		return &_gpio->Port23;
	}
	else if (gio < 96)
	{
		*pmask = (1<<(gio - 64));
		return &_gpio->Port45;
	}
	else if (gio < 104)
	{
		*pmask = (1<<(gio - 96));
		return &_gpio->Port6;
	}
	else
	{
		*pmask = 0;
		return NULL;
	}
}

void gpio_setup(int gio, GPIO_DIR dir, int value)
{
	unsigned long mask;
	GPIO_PORT *port = _gio(gio, &mask);
	if (port != NULL)
	{
		if (dir == GPIO_DIR_INPUT)
		{
			port->DIR |= mask;
		}
		else
		{
			if (value)
			{
				port->SET_DATA = mask;
			}
			else
			{
				port->CLR_DATA = mask;
			}
			port->DIR &= ~mask;
		}
	}
}

void gpio_set(int gio, int value)
{
	unsigned long mask;
	GPIO_PORT *port = _gio(gio, &mask);
	if (port != NULL)
	{
		if (value)
		{
			port->SET_DATA = mask;
		}
		else
		{
			port->CLR_DATA = mask;
		}
	}
}

