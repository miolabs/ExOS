#ifndef STM32F4_DAC_H
#define STM32F4_DAC_H

typedef enum
{
	STM32_DAC_TRIGGER_TMR6 = 0x0,
	STM32_DAC_TRIGGER_TMR8,
	STM32_DAC_TRIGGER_TMR7,
	STM32_DAC_TRIGGER_TMR5,
	STM32_DAC_TRIGGER_TMR2,
	STM32_DAC_TRIGGER_TMR4,
	STM32_DAC_TRIGGER_EXTI9,
	STM32_DAC_TRIGGER_SW
} stm32_dac_trigger_t;

typedef enum
{
	STM32_DACF_BUF_OFF = (1<<0),	// buffer provides low impedance output
	STM32_DACF_UDRIE = (1<<1),		// under-run interrupt enable
	STM32_DACF_DMA = (1<<2),
	STM32_DACF_TEN = (1<<3),
	STM32_DACF_INITIAL = (1<<4),
} stm32_dac_flags_t;


void dac_initialize(unsigned ch, stm32_dac_trigger_t trig, stm32_dac_flags_t flags, unsigned short initial_value);
void dac_write16(unsigned ch, unsigned short value);

#endif // STM32F4_DAC_H


