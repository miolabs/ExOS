#ifndef STM32F4_GPIO_H
#define STM32F4_GPIO_H

#include <stm32f4xx.h>

#define GPIO_MODE_SPEED_BIT 8
#define GPIO_MODE_SPEED_MASK (3<<GPIO_MODE_SPEED_BIT)

typedef enum
{
	GPIO_MODE_INPUT = 0,
	GPIO_MODE_OUTPUT = 1,
	GPIO_MODE_ALT_FUNC = 2,
	GPIO_MODE_ANALOG = 3,

	GPIO_MODEF_OPEN_DRAIN = (1<<2),
	GPIO_MODEF_PULL_UP = (1<<3),
	GPIO_MODEF_PULL_DOWN = (1<<4),
	
	GPIO_MODEF_LOW_SPEED = (0<<GPIO_MODE_SPEED_BIT),
	GPIO_MODEF_MED_SPEED = (1<<GPIO_MODE_SPEED_BIT),
	GPIO_MODEF_FAST_SPEED = (2<<GPIO_MODE_SPEED_BIT),
	GPIO_MODEF_HIGH_SPEED = (3<<GPIO_MODE_SPEED_BIT),
} gpio_mode_t;

typedef enum
{
	GPIO_PORTF_A = (1<<0),
	GPIO_PORTF_B = (1<<1),
	GPIO_PORTF_C = (1<<2),
	GPIO_PORTF_D = (1<<3),
	GPIO_PORTF_E = (1<<4),
	GPIO_PORTF_F = (1<<5),
	GPIO_PORTF_G = (1<<6),
	GPIO_PORTF_H = (1<<7),
	GPIO_PORTF_I = (1<<8),
	GPIO_PORTF_J = (1<<9),
	GPIO_PORTF_K = (1<<10),
} gpio_port_mask_t;

typedef enum
{
	PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
	PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
	PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
	PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
	PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
	PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
	PG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
	PH0, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,
	PI0, PI1, PI2, PI3, PI4, PI5, PI6, PI7, PI8, PI9, PI10, PI11, PI12, PI13, PI14, PI15,
	PJ0, PJ1, PJ2, PJ3, PJ4, PJ5, PJ6, PJ7, PJ8, PJ9, PJ10, PJ11, PJ12, PJ13, PJ14, PJ15,
	PK0, PK1, PK2, PK3, PK4, PK5, PK6, PK7, PK8, PK9, PK10, PK11, PK12, PK13, PK14, PK15,
} gpio_pin_t;

void gpio_port_enable(gpio_port_mask_t mask);
void gpio_pin_config(gpio_pin_t pin, gpio_mode_t mode, unsigned alt_func);

#endif // STM32F4_GPIO_H

