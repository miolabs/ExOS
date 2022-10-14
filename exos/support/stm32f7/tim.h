#ifndef STM32F7_TIMER_H
#define STM32F7_TIMER_H

#include "cpu.h"
#include <support/pwm_hal.h>
#include <stdbool.h>

#define TIM_OC_MODE_FROZEN 0
#define TIM_OC_MODE_HIGH 1
#define TIM_OC_MODE_LOW 2
#define TIM_OC_MODE_TOGGLE 3
#define TIM_OC_MODE_FORCE_LOW 4
#define TIM_OC_MODE_FORCE_HIGH 5
#define TIM_OC_MODE_PWM_MODE1 6
#define TIM_OC_MODE_PWM_MODE2 7


typedef void (*timer_handler_t)(unsigned module, unsigned channel);

typedef enum
{
	TIMER_MATF_INTERRUPT = (1<<0),
	TIMER_MATF_RESET = (1<<1),
	TIMER_MATF_STOP = (1<<2),
} mat_flags_t;

typedef enum
{
	TIMER_CAPF_INTERRUPT = (1<<0),
	TIMER_CAPF_RISE = (1<<1),
	TIMER_CAPF_FALL = (1<<2),
	TIMER_CAPF_TOGGLE = (TIMER_CAPF_RISE | TIMER_CAPF_RISE),

} cap_flags_t;

typedef enum
{
	TIMER_CTRLF_NONE = 0,
	TIMER_CTRLF_RESET = (1<<0),
	TIMER_CTRLF_START = (1<<1),
	TIMER_CTRLF_STOP = (1<<2),
} timer_ctrl_flags_t;

void timer_initialize(unsigned module, unsigned long freq, timer_handler_t handler, timer_ctrl_flags_t flags);
void timer_control(unsigned module, timer_ctrl_flags_t flags);
unsigned long timer_value(unsigned module);
void timer_capture_setup(unsigned module, unsigned channel, cap_flags_t flags);
void timer_match_setup(unsigned module, unsigned channel, unsigned long value, mat_flags_t flags);


#endif // STM32F7_TIMER_H


