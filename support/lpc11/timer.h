#ifndef LPC11_TIMER32_H
#define LPC11_TIMER32_H

#include "cpu.h"

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


#endif // LPC11_TIMER32_H



