#ifndef STM32F7_EXTI_H
#define STM32F7_EXTI_H

typedef enum
{
	EXTI_PORTA = 0,
	EXTI_PORTB,
	EXTI_PORTC,
	EXTI_PORTD,
	EXTI_PORTE,
	EXTI_PORTF,
	EXTI_PORTG,
	EXTI_PORTH,
	EXTI_PORTI,
	EXTI_PORTJ,
	EXTI_PORTK,
} exti_port_t;

typedef enum
{
	EXTIF_NONE = 0,
	EXTIF_RISING = (1<<0),
	EXTIF_FALLING = (1<<1),
} exti_flags_t;

typedef void(*exti_handler_t)(unsigned index);

// prototypes
unsigned exti_pin_config(unsigned pin, exti_flags_t flags, exti_handler_t handler);
void exti_edge_config(unsigned index, exti_flags_t flags, exti_handler_t handler);

#endif // STM32F7_EXTI_H


