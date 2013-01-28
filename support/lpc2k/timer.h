#ifndef LPC_TIMER_H
#define LPC_TIMER_H

#include <support/lpc2k/cpu.h>

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

extern LPC_TIM_TypeDef *LPC_TIM0;
extern LPC_TIM_TypeDef *LPC_TIM1;
extern LPC_TIM_TypeDef *LPC_TIM2;
extern LPC_TIM_TypeDef *LPC_TIM3;


typedef enum
{
	CAPM_RISING = 1,
	CAPM_FALLING = 2,
	CAPM_INT = 4,
} TIMER_CAPM;

typedef struct
{
	TIMER_CAPM CR0:3;
	TIMER_CAPM CR1:3;
} TIMER_CCR;

#define TIMER_IR_MR0 (1<<0)
#define TIMER_IR_MR1 (1<<1)
#define TIMER_IR_MR2 (1<<2)
#define TIMER_IR_MR3 (1<<3)
#define TIMER_IR_CR0 (1<<4)
#define TIMER_IR_CR1 (1<<5)

#define TIMER_MR_INT (1<<0)
#define TIMER_MR_RESET (1<<1)
#define TIMER_MR_STOP (1<<2)

typedef enum
{
	TIMER_MODE_DISABLED = 0,
	TIMER_MODE_CONTINUOUS,
	TIMER_MODE_ONCE,
	TIMER_MODE_CONTINUOUS_RELOAD,
} TIMER_MODE;

typedef void (* MATCH_HANDLER)(int module);

void timer_initialize(int module, int freq, int period, TIMER_MODE mode, MATCH_HANDLER fn, int pri);

#endif // LPC_TIMER_H


