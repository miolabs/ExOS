#ifndef LPC_TIMER_H
#define LPC_TIMER_H

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

#endif // LPC_TIMER_H


