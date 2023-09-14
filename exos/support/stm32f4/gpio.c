#include "gpio.h"
#include "cpu.h"
#include <support/gpio_hal.h>
#include <kernel/panic.h>

#ifndef GPIOF
#define GPIOF nullptr
#endif
#ifndef GPIOG
#define GPIOG nullptr
#endif
#ifndef GPIOI
#define GPIOI nullptr
#endif
#ifndef GPIOJ
#define GPIOJ nullptr
#endif
#ifndef GPIOK
#define GPIOK nullptr
#endif

static GPIO_TypeDef *const _ports[] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK };
static const unsigned _port_count = sizeof(_ports) >> 2;
#define GPIO_PORT_COUNT _port_count

void gpio_port_enable(gpio_port_mask_t mask)
{
	unsigned ahb1_enr = 0;
	if (mask & GPIO_PORTF_A)
		ahb1_enr |= RCC_AHB1ENR_GPIOAEN;
	if (mask & GPIO_PORTF_B)
		ahb1_enr |= RCC_AHB1ENR_GPIOBEN;
	if (mask & GPIO_PORTF_C)
		ahb1_enr |= RCC_AHB1ENR_GPIOCEN;
	if (mask & GPIO_PORTF_D)
		ahb1_enr |= RCC_AHB1ENR_GPIODEN;
	if (mask & GPIO_PORTF_E)
		ahb1_enr |= RCC_AHB1ENR_GPIOEEN;
#ifdef RCC_AHB1ENR_GPIOFEN
	if (mask & GPIO_PORTF_F)
		ahb1_enr |= RCC_AHB1ENR_GPIOFEN;
#endif
#ifdef RCC_AHB1ENR_GPIOGEN
	if (mask & GPIO_PORTF_G)
		ahb1_enr |= RCC_AHB1ENR_GPIOGEN;
#endif
	if (mask & GPIO_PORTF_H)
		ahb1_enr |= RCC_AHB1ENR_GPIOHEN;
#ifdef RCC_AHB1ENR_GPIOIEN
	if (mask & GPIO_PORTF_I)
		ahb1_enr |= RCC_AHB1ENR_GPIOIEN;
#endif
#ifdef RCC_AHB1ENR_GPIOJEN
	if (mask & GPIO_PORTF_J)
		ahb1_enr |= RCC_AHB1ENR_GPIOJEN;
#endif
#ifdef RCC_AHB1ENR_GPIOKEN
	if (mask & GPIO_PORTF_K)
		ahb1_enr |= RCC_AHB1ENR_GPIOKEN;
#endif
	if (ahb1_enr != 0)
		RCC->AHB1ENR |= ahb1_enr;
}

void gpio_pin_config(gpio_pin_t pin, gpio_mode_t mode, unsigned alt_func)
{
	unsigned port = pin >> 4;
	ASSERT(port < GPIO_PORT_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	GPIO_TypeDef *gpio = _ports[port];
	ASSERT(gpio != nullptr, KERNEL_ERROR_KERNEL_PANIC);

	unsigned altf_shift = (pin & 0x7) << 2;
	unsigned altf_mask = 0xf << altf_shift;
	unsigned afrr = (pin >> 3) & 1;
	gpio->AFR[afrr] = (gpio->AFR[afrr] & ~altf_mask) | ((alt_func & 0xf) << altf_shift);

	unsigned mode_shift = (pin & 0xf) << 1;
	unsigned mode_mask = 0x3 << mode_shift;
	gpio->MODER = (gpio->MODER & ~mode_mask) | ((mode & 0x3) << mode_shift);

	unsigned odr_mask = 1 << (pin & 0xf);
	if (mode & GPIO_MODEF_OPEN_DRAIN)
		gpio->OTYPER |= odr_mask;
	else
		gpio->OTYPER &= ~odr_mask;

	unsigned ospd = (mode & GPIO_MODE_SPEED_MASK) >> GPIO_MODE_SPEED_BIT;
	gpio->OSPEEDR = (gpio->OSPEEDR & ~mode_mask) | (ospd << mode_shift); 

	unsigned pupd = ((mode & GPIO_MODEF_PULL_UP) ? 1 : 0) | ((mode & GPIO_MODEF_PULL_DOWN) ? 2 : 0);
	gpio->PUPDR = (gpio->PUPDR & ~mode_mask) | ((pupd << mode_shift) & mode_mask);
}

void hal_gpio_pin_set(unsigned pin, bool state)
{
	unsigned port = pin >> 4;
	ASSERT(port < GPIO_PORT_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	GPIO_TypeDef *gpio = _ports[port];
	ASSERT(gpio != nullptr, KERNEL_ERROR_KERNEL_PANIC);
	
	unsigned mask = 1 << (pin & 0xf);
	if (state)
		gpio->BSRR = mask;
	else
		gpio->BSRR = mask << 16;
}

bool hal_gpio_pin(unsigned pin)
{
	unsigned port = pin >> 4;
	ASSERT(port < GPIO_PORT_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	GPIO_TypeDef *gpio = _ports[port];
	ASSERT(gpio != nullptr, KERNEL_ERROR_KERNEL_PANIC);
		
	unsigned mask = 1 << (pin & 0xf);
	return gpio->IDR & mask;
}

void hal_gpio_pin_config(unsigned pin, hal_gpio_flags_t flags)
{
	gpio_mode_t mode = (flags & GPIOF_ANALOG) ? GPIO_MODE_ANALOG :
		((flags & GPIOF_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
	if (flags & GPIOF_PULLUP) mode |= GPIO_MODEF_PULL_UP;
	if (flags & GPIOF_PULLDOWN) mode |= GPIO_MODEF_PULL_DOWN;
	if (flags & GPIOF_OPEN_DRAIN) mode |= GPIO_MODEF_OPEN_DRAIN;
	gpio_pin_config(pin, mode, 0);
}

