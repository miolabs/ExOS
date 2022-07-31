#include "timer.h"
#include "system.h"

#define TIMER_MODULE_COUNT 5

static TIMER_MODULE *_timers[] = { (TIMER_MODULE *)0x01C21400, 
	(TIMER_MODULE *)0x01C21800, (TIMER_MODULE *)0x01C21C00, 
	(TIMER_MODULE *)0x01C20800, (TIMER_MODULE *)0x01C23800 };

static MATCH_FN _handlers[TIMER_MODULE_COUNT];

TIMER_MODULE *timer_initialize(int timer, int freq, int period, TIMER_MODE mode)
{
	TIMER_MODULE *module = _timers[timer];
	module->TCR = 0;	// disable all
	_handlers[timer] = NULL;

	module->TGCR = (TGCR_TIMMODE_DUAL_CHAINED << TGCR_TIMMODE_BIT) | TGCR_TIM12RS | TGCR_TIM34RS;
	
	module->PRD34 = (OSCILLATOR_CLOCK_FREQUENCY / freq) - 1;
	
	module->PRD12 = period - 1;
	module->TCRbits.ENAMODE12 = mode;

	// timer is stopped, waiting TIM34 (prescaler) to be started
	return module;
}

void timer_control(int timer, int enable)
{
	TIMER_MODULE *module = _timers[timer];
	module->TCRbits.ENAMODE34 = enable ? TIMER_MODE_CONTINUOUS : TIMER_MODE_DISABLED;
}

void timer_set_handler(int timer, MATCH_FN fn, INTC_PRI pri)
{
	switch(timer)
	{
		case 0:
			intc_set_priority(32, 1, pri);
			break;
	}
	_handlers[timer] = fn;
}

// Timer0 - TINT12
void INT32_Handler()
{
	MATCH_FN handler = _handlers[0];
	if (handler != NULL)
	{
		TIMER_MODULE *module = _timers[0];
		handler(0, module->TIM12);
	}
}