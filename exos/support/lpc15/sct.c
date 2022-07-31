// State Configurable Timer modules support for LPC15xx
// by Miguel Fides

#include "sct.h"
#include "cpu.h"

#pragma GCC optimize(2)

const SCT_MATCH __sct_match_end = { .Flags = SCT_MATF_END };

static SCT_HANDLER _handler[4];

static void _init_big_sct(LPC_SCT0_Type *sct, unsigned int freq, unsigned int period)
{
	unsigned long base_freq = SystemCoreClock;
	int div = (base_freq + (freq/2)) / freq;
	
	// FIXME: always unified, bidirectional
	sct->CONFIG = SCT_CONFIG_UNIFY | SCT_CONFIG_CLKMODE_SYS; 
	sct->CTRL = SCT_CTRL_HALT_L | SCT_CTRL_CLRCTR_L | SCT_CTRL_BIDIR_L |
		((div - 1) << SCT_CTRL_PRE_L_BIT);
	
	if (period != 0)
	{
		sct->MATCH0 = sct->MATCHREL0 = period - 1;
		sct->CONFIG |= SCT_CONFIG_AUTOLIMIT_L;
	}
}

void sct_initialize(int module, unsigned int freq, unsigned int period, SCT_HANDLER handler)
{
	LPC_SYSCON->SCTPLLCLKSEL = 1;	// Use system oscillator as SCT base clock

	switch(module)
	{
		case 0:
			LPC_SYSCON->PRESETCTRL1 |= PRESETCTRL1_SCT0;
			LPC_SYSCON->SYSAHBCLKCTRL1 |= CLKCTRL1_SCT0;
			LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL1_SCT0;
			_init_big_sct(LPC_SCT0, freq, period);
			NVIC_EnableIRQ(SCT0_IRQn);
			break;
		case 1:
			LPC_SYSCON->SYSAHBCLKCTRL1 |= CLKCTRL1_SCT1;
			LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL1_SCT1;
			_init_big_sct(LPC_SCT1, freq, period);
			NVIC_EnableIRQ(SCT1_IRQn);
			break;
		case 2:
			LPC_SYSCON->SYSAHBCLKCTRL1 |= CLKCTRL1_SCT2;
			LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL1_SCT2;
//			_init_small_sct(LPC_SCT2);
			NVIC_EnableIRQ(SCT2_IRQn);
			break;
		case 3:
			LPC_SYSCON->SYSAHBCLKCTRL1 |= CLKCTRL1_SCT3;
			LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL1_SCT3;
//			_init_small_sct(LPC_SCT3);
			NVIC_EnableIRQ(SCT3_IRQn);
			break;
		default:
			return;
	}

	_handler[module] = handler;
}

static void _control_big(LPC_SCT0_Type *sct, SCT_CTRL_FLAGS flags)
{
	if (flags & SCT_CTRLF_RELOAD)
	{
		sct->MATCH0 = sct->MATCHREL0;
		sct->MATCH1 = sct->MATCHREL1;
		sct->MATCH2 = sct->MATCHREL2;
		sct->MATCH3 = sct->MATCHREL3;
		sct->MATCH4 = sct->MATCHREL4;
		sct->MATCH5 = sct->MATCHREL5;
		sct->MATCH6 = sct->MATCHREL6;
		sct->MATCH7 = sct->MATCHREL7;
		sct->MATCH8 = sct->MATCHREL8;
		sct->MATCH9 = sct->MATCHREL9;
		sct->MATCH10 = sct->MATCHREL10;
		sct->MATCH11 = sct->MATCHREL11;
		sct->MATCH12 = sct->MATCHREL12;
		sct->MATCH13 = sct->MATCHREL13;
		sct->MATCH14 = sct->MATCHREL14;
		sct->MATCH15 = sct->MATCHREL15;
	}

	if (flags & (SCT_CTRLF_RUN|SCT_CTRLF_STOP|SCT_CTRLF_HALT))
	{
		unsigned int ctrl = sct->CTRL & ~(SCT_CTRL_STOP_L | SCT_CTRL_HALT_L);
		if (flags & SCT_CTRLF_STOP) ctrl |= SCT_CTRL_STOP_L;
		if (flags & SCT_CTRLF_HALT) ctrl |= SCT_CTRL_HALT_L;
		sct->CTRL = ctrl;
	}

}

void sct_control(int module, SCT_CTRL_FLAGS flags)
{
	switch(module)
	{
		case 0: _control_big(LPC_SCT0, flags); break;
		case 1: _control_big(LPC_SCT1, flags); break;
		// TODO
	}
}

static void _setup_event_big(LPC_SCT0_Type *sct, int event, SCT_EV_SETUP *setup, int state_mask)
{
	unsigned int mask = (1 << event);
	SCT_EV_DIRECTION dir = setup->Flags & (SCT_EVF_ONLY_COUNTING_UP | SCT_EVF_ONLY_COUNTING_DOWN) ?
		(setup->Flags & SCT_EVF_ONLY_COUNTING_UP ? SCT_EV_DIR_UP : SCT_EV_DIR_DOWN) : SCT_EV_DIR_ANY;

	unsigned int ctrl = (setup->Match << SCT_EVCTRL_MATCHSEL_BIT) |
		(setup->Flags & SCT_EVF_OUTPUT ? SCT_EVCTRL_OUTSEL : 0) |
		(setup->IOSel << SCT_EVCTRL_IOSEL_BIT) |
		(setup->IOCond << SCT_EVCTRL_IOCOND_BIT) |
		(setup->CombMode << SCT_EVCTRL_COMBMODE_BIT) |
		(setup->Flags & SCT_EVF_STATE_LOAD ? SCT_EVCTRL_STATELD : 0) |
		(setup->StateVal << SCT_EVCTRL_STATEV_BIT) |
		(setup->Flags & SCT_EVF_MATCH_MEMORY ? SCT_EVCTRL_MATCHMEM : 0) |
		(dir << SCT_EVCTRL_DIRECTION_BIT);

	switch(event)
	{
		case 0:		
			sct->EV0_CTRL = ctrl;
			sct->EV0_STATE = state_mask;
			break;
		case 1:		
			sct->EV1_CTRL = ctrl;	
			sct->EV1_STATE = state_mask;
			break;
		case 2:		
			sct->EV2_CTRL = ctrl;	
			sct->EV2_STATE = state_mask;
			break;
		case 3:
			sct->EV3_CTRL = ctrl;
			sct->EV3_STATE = state_mask;
			break;
		case 4:
			sct->EV4_CTRL = ctrl;
			sct->EV4_STATE = state_mask;
			break;
		case 5:
			sct->EV5_CTRL = ctrl;
			sct->EV5_STATE = state_mask;
			break;
		case 6:
			sct->EV6_CTRL = ctrl;
			sct->EV6_STATE = state_mask;
			break;
		case 7:
			sct->EV7_CTRL = ctrl;
			sct->EV7_STATE = state_mask;
			break;
		case 8:
			sct->EV8_CTRL = ctrl;
			sct->EV8_STATE = state_mask;
			break;
		case 9:
			sct->EV9_CTRL = ctrl;
			sct->EV9_STATE = state_mask;
			break;
		case 10:
			sct->EV10_CTRL = ctrl;
			sct->EV10_STATE = state_mask;
			break;
		case 11:
			sct->EV11_CTRL = ctrl;
			sct->EV11_STATE = state_mask;
			break;
		case 12:
			sct->EV12_CTRL = ctrl;
			sct->EV12_STATE = state_mask;
			break;
		case 13:
			sct->EV13_CTRL = ctrl;
			sct->EV13_STATE = state_mask;
			break;
		case 14:
			sct->EV14_CTRL = ctrl;
			sct->EV14_STATE = state_mask;
			break;
		case 15:
			sct->EV15_CTRL = ctrl;
			sct->EV15_STATE = state_mask;
			break;
	}

	if (setup->Flags & SCT_EVF_LIMIT) 
		sct->LIMIT |= mask;
	else 
		sct->LIMIT &= ~mask;

	if (setup->Flags & SCT_EVF_HALT) 
		sct->HALT |= mask;
	else 
		sct->HALT &= ~mask;

	if (setup->Flags & SCT_EVF_STOP) 
		sct->STOP |= mask;
	else 
		sct->STOP &= ~mask;

	if (setup->Flags & SCT_EVF_START) 
		sct->START |= mask;
	else 
		sct->START &= ~mask;

	if (setup->Flags & SCT_EVF_IRQEN)
		sct->EVEN |= mask;
	else
		sct->EVEN &= ~mask;

}

void sct_event_setup(int module, int event, SCT_EV_SETUP *setup, int state_mask)
{
	switch(module)
	{
		case 0:	_setup_event_big(LPC_SCT0, event, setup, state_mask);	break;
		case 1:	_setup_event_big(LPC_SCT1, event, setup, state_mask);	break;
//		case 2:	_setup_event_small(LPC_SCT2, event, setup);	break;
//		case 3:	_setup_event_small(LPC_SCT3, event, setup);	break;
	}
}

static void _setup_output_big(LPC_SCT0_Type *sct, int output, unsigned int event_mask, SCT_OUT_FLAGS flags)
{
	int setclr_bit = (output & 0xf) << 1;
	sct->OUTPUTDIRCTRL &= ~(0x3 << setclr_bit);
	if (flags & SCT_OUTF_COUNTDOWN_REVERSE)
		sct->OUTPUTDIRCTRL |= (1 << setclr_bit);

	switch(output)
	{
		case 0:
			if (flags & SCT_OUTF_SET) sct->OUT0_SET |= event_mask;
				else sct->OUT0_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT0_CLR |= event_mask;
				else sct->OUT0_CLR != ~event_mask;
			break;
		case 1:
			if (flags & SCT_OUTF_SET) sct->OUT1_SET |= event_mask;
				else sct->OUT1_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT1_CLR |= event_mask;
				else sct->OUT1_CLR != ~event_mask;
			break;
		case 2:
			if (flags & SCT_OUTF_SET) sct->OUT2_SET |= event_mask;
				else sct->OUT2_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT2_CLR |= event_mask;
				else sct->OUT2_CLR != ~event_mask;
			break;
		case 3:
			if (flags & SCT_OUTF_SET) sct->OUT3_SET |= event_mask;
				else sct->OUT3_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT3_CLR |= event_mask;
				else sct->OUT3_CLR != ~event_mask;
			break;
		case 4:
			if (flags & SCT_OUTF_SET) sct->OUT4_SET |= event_mask;
				else sct->OUT4_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT4_CLR |= event_mask;
				else sct->OUT4_CLR != ~event_mask;
			break;
		case 5:
			if (flags & SCT_OUTF_SET) sct->OUT5_SET |= event_mask;
				else sct->OUT5_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT5_CLR |= event_mask;
				else sct->OUT5_CLR != ~event_mask;
			break;
		case 6:
			if (flags & SCT_OUTF_SET) sct->OUT6_SET |= event_mask;
				else sct->OUT6_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT6_CLR |= event_mask;
				else sct->OUT6_CLR != ~event_mask;
			break;
		case 7:
			if (flags & SCT_OUTF_SET) sct->OUT7_SET |= event_mask;
				else sct->OUT7_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT7_CLR |= event_mask;
				else sct->OUT7_CLR != ~event_mask;
			break;
		case 8:
			if (flags & SCT_OUTF_SET) sct->OUT8_SET |= event_mask;
				else sct->OUT8_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT8_CLR |= event_mask;
				else sct->OUT8_CLR != ~event_mask;
			break;
		case 9:
			if (flags & SCT_OUTF_SET) sct->OUT9_SET |= event_mask;
				else sct->OUT9_SET != ~event_mask;
			if (flags & SCT_OUTF_CLR) sct->OUT9_CLR |= event_mask;
				else sct->OUT9_CLR != ~event_mask;
			break;
	}

	unsigned int conf_bit = output * 2;
	unsigned int conf_val = ((flags & SCT_OUTF_CONFLICT_SET) ? SCT_RES_SET : 0) | ((flags & SCT_OUTF_CONFLICT_CLR) ? SCT_RES_CLR : 0);
	sct->RES = (sct->RES & ~(3 << conf_bit)) | (conf_val << conf_bit);
}

void sct_output_setup(int module, int output, unsigned int event_mask, SCT_OUT_FLAGS flags)
{
	switch(module)
	{
		case 0:	_setup_output_big(LPC_SCT0, output, event_mask, flags);	break;
		case 1:	_setup_output_big(LPC_SCT1, output, event_mask, flags);	break;
	}
}

static void _setup_match_big(LPC_SCT0_Type *sct, int match, unsigned int value)
{
	switch(match)
	{
		case 0: sct->MATCHREL0 = value;	break;
		case 1: sct->MATCHREL1 = value;	break;
		case 2: sct->MATCHREL2 = value;	break;
		case 3: sct->MATCHREL3 = value;	break;
		case 4: sct->MATCHREL4 = value;	break;
		case 5: sct->MATCHREL5 = value;	break;
		case 6: sct->MATCHREL6 = value;	break;
		case 7: sct->MATCHREL7 = value;	break;
		case 8: sct->MATCHREL8 = value;	break;
		case 9: sct->MATCHREL9 = value;	break;
		case 10: sct->MATCHREL10 = value;	break;
		case 11: sct->MATCHREL11 = value;	break;
		case 12: sct->MATCHREL12 = value;	break;
		case 13: sct->MATCHREL13 = value;	break;
		case 14: sct->MATCHREL14 = value;	break;
		case 15: sct->MATCHREL15 = value;	break;
	}
}

void sct_match_set(int module, int match, unsigned int value)
{
	switch(module)
	{
		case 0:	_setup_match_big(LPC_SCT0, match, value);	break;
		case 1:	_setup_match_big(LPC_SCT1, match, value);	break;
	}
}

static void _setup_match_array_big(LPC_SCT0_Type *sct, SCT_MATCH *array)
{
	sct->CONFIG |= SCT_CONFIG_NORELOAD_L;

	for(SCT_MATCH *match = array; !(match->Flags & SCT_MATF_END); match++)
	{
		_setup_match_big(sct, match->Match, match->Value);
	}

	sct->CONFIG &= ~SCT_CONFIG_NORELOAD_L;
}

void sct_match_set_array(int module, SCT_MATCH *array)
{
	switch(module)
	{
		case 0:	_setup_match_array_big(LPC_SCT0, array);	break;
		case 1:	_setup_match_array_big(LPC_SCT1, array);	break;
	}
}

void SCT0_IRQHandler()
{
	unsigned int flags = LPC_SCT0->EVFLAG;
	SCT_HANDLER hnd = _handler[0];
	if (hnd) hnd(0);
	LPC_SCT0->EVFLAG = flags;
}

void SCT1_IRQHandler()
{
	unsigned int flags = LPC_SCT1->EVFLAG;
	SCT_HANDLER hnd = _handler[1];
	if (hnd) hnd(1);
   	LPC_SCT1->EVFLAG = flags;
}

void SCT2_IRQHandler()
{
	SCT_HANDLER hnd = _handler[2];
	if (hnd) hnd(2);
}

void SCT3_IRQHandler()
{
	SCT_HANDLER hnd = _handler[3];
	if (hnd) hnd(3);
}



