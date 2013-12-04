#ifndef LPC11_TIMER16_H
#define LPC11_TIMER16_H

#include "cpu.h"

#define TMR16_IR_CAP0 (1<<4)

#define TMR16_MCR_MR0I (1<<0)
#define TMR16_MCR_MR0R (1<<1)
#define TMR16_MCR_MR0S (1<<2)
#define TMR16_MCR_MR1I (1<<3)
#define TMR16_MCR_MR1R (1<<4)
#define TMR16_MCR_MR1S (1<<5)
#define TMR16_MCR_MR2I (1<<6)
#define TMR16_MCR_MR2R (1<<7)
#define TMR16_MCR_MR2S (1<<8)
#define TMR16_MCR_MR3I (1<<9)
#define TMR16_MCR_MR3R (1<<10)
#define TMR16_MCR_MR3S (1<<11)

#define TMR16_CCR_CAP0RE	(1<<0)
#define TMR16_CCR_CAP0FE	(1<<1)
#define TMR16_CCR_CAP0I		(1<<2)

#endif // LPC11_TIMER16_H