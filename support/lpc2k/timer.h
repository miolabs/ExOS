#ifndef LPC_TIMER_H
#define LPC_TIMER_H

#include <support/lpc2k/cpu.h>
#include <support/pwm_hal.h>

/*------------- Timer (TIM) --------------------------------------------------*/
typedef struct
{
  __IO unsigned long IR;
  __IO unsigned long TCR;
  __IO unsigned long TC;
  __IO unsigned long PR;
  __IO unsigned long PC;
  __IO unsigned long MCR;
  __IO unsigned long MR0;
  __IO unsigned long MR1;
  __IO unsigned long MR2;
  __IO unsigned long MR3;
  __IO unsigned long CCR;
  __I  unsigned long CR0;
  __I  unsigned long CR1;
       unsigned long RESERVED0[2];
  __IO unsigned long EMR;
       unsigned long RESERVED1[12];
  __IO unsigned long CTCR;
} LPC_TIM_TypeDef;

#define LPC_TIM0_BASE	0xE0004000
#define LPC_TIM1_BASE	0xE0008000
#define LPC_TIM2_BASE	0xE0070000
#define LPC_TIM3_BASE	0xE0074000

#define LPC_TIM0 ((LPC_TIM_TypeDef *)LPC_TIM0_BASE)
#define LPC_TIM1 ((LPC_TIM_TypeDef *)LPC_TIM1_BASE)
#define LPC_TIM2 ((LPC_TIM_TypeDef *)LPC_TIM2_BASE)
#define LPC_TIM3 ((LPC_TIM_TypeDef *)LPC_TIM3_BASE)

#define TIMER_IR_MAT0 (1<<0)
#define TIMER_IR_MAT1 (1<<1)
#define TIMER_IR_MAT2 (1<<2)
#define TIMER_IR_MAT3 (1<<3)
#define TIMER_IR_CAP0 (1<<4)
#define TIMER_IR_CAP1 (1<<5)

#define TIMER_MCR_MR0I (1<<0)
#define TIMER_MCR_MR0R (1<<1)
#define TIMER_MCR_MR0S (1<<2)
#define TIMER_MCR_MR1I (1<<3)
#define TIMER_MCR_MR1R (1<<4)
#define TIMER_MCR_MR1S (1<<5)
#define TIMER_MCR_MR2I (1<<6)
#define TIMER_MCR_MR2R (1<<7)
#define TIMER_MCR_MR2S (1<<8)
#define TIMER_MCR_MR3I (1<<9)
#define TIMER_MCR_MR3R (1<<10)
#define TIMER_MCR_MR3S (1<<11)

#define TIMER_CCR_CAP0RE	(1<<0)
#define TIMER_CCR_CAP0FE	(1<<1)
#define TIMER_CCR_CAP0I		(1<<2)

#define TIMER_EMR_EM0 (1<<0)
#define TIMER_EMR_EM1 (1<<1)
#define TIMER_EMR_EM2 (1<<2)
#define TIMER_EMR_EM3 (1<<3)
#define TIMER_EMR_EMC0_BIT (4)
#define TIMER_EMR_EMC0_MASK (3<<TIMER_EMR_EMC0_BIT)
#define TIMER_EMR_EMC1_BIT (6)
#define TIMER_EMR_EMC1_MASK (3<<TIMER_EMR_EMC1_BIT)
#define TIMER_EMR_EMC2_BIT (8)
#define TIMER_EMR_EMC2_MASK (3<<TIMER_EMR_EMC2_BIT)
#define TIMER_EMR_EMC3_BIT (10)
#define TIMER_EMR_EMC3_MASK (3<<TIMER_EMR_EMC3_BIT)

#define TIMER_EMC_NOTHING 0
#define TIMER_EMC_CLEAR 1
#define TIMER_EMC_SET 2
#define TIMER_EMC_TOGGLE 3

int timer_match_initialize(int module, int range, int rate, int channel_for_period);
void timer_match_set_handler(int module, int channel, HAL_PWM_HANDLER callback);
void timer_match_set_value(int module, int channel, int value);
void timer_match_set_trigger(int module, int channel, int trigger, int state);
unsigned long timer_actual(int module);

#endif // LPC_TIMER_H


