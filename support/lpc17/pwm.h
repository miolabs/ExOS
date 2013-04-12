#ifndef LPC17_PWM_H
#define LPC17_PWM_H

#define PWM_IR_MR0 (1<<0)
#define PWM_IR_MR1 (1<<1)
#define PWM_IR_MR2 (1<<2)
#define PWM_IR_MR3 (1<<3)
#define PWM_IR_CR0 (1<<4)
#define PWM_IR_CR1 (1<<5)
#define PWM_IR_MR4 (1<<8)
#define PWM_IR_MR5 (1<<9)
#define PWM_IR_MR6 (1<<10)
#define PWM_IR_MR_MASK (PWM_IR_MR0 | PWM_IR_MR1 | PWM_IR_MR2 | PWM_IR_MR3 | PWM_IR_MR4 | PWM_IR_MR5 | PWM_IR_MR6)

#define PWM_TCR_RUN (1<<0)
#define PWM_TCR_RESET (1<<1)
#define PWM_TCR_PWMEN (1<<3)

#define PWM_PCR_PWMSEL2 (1<<2)
#define PWM_PCR_PWMSEL3 (1<<3)
#define PWM_PCR_PWMSEL4 (1<<4)
#define PWM_PCR_PWMSEL5 (1<<5)
#define PWM_PCR_PWMSEL6 (1<<6)
#define PWM_PCR_PWMENA1 (1<<9)
#define PWM_PCR_PWMENA2 (1<<10)
#define PWM_PCR_PWMENA3 (1<<11)
#define PWM_PCR_PWMENA4 (1<<12)
#define PWM_PCR_PWMENA5 (1<<13)
#define PWM_PCR_PWMENA6 (1<<14)

#endif // LPC17_PWM_H
