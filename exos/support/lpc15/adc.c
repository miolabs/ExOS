// Internal AD module support for LPC15xx
// by Miguel Fides

#include "adc.h"

#ifndef LPC_ADC_BASE_FREQ
#define LPC_ADC_BASE_FREQ SystemCoreClock
#endif

static void _calibrate(LPC_ADC0_Type *adc)
{
	int freq = 500000;
	int div = (SystemCoreClock + (freq - 1)) / freq;
	adc->CTRL = ((div - 1) << ADC_CTRL_CLKDIV_BIT) | ADC_CTRL_CALMODE;
	int wait = 0;
	while(adc->CTRL & ADC_CTRL_CALMODE) wait++;
}

static void _init(LPC_ADC0_Type *adc, int sample_rate, LPC_ADC_INIT_FLAGS flags)
{
	_calibrate(adc);

	int freq = sample_rate * (flags & LPC_ADC_INITF_FAST ? 18 : 25); 
    int max_freq = flags & LPC_ADC_INITF_FAST ? 72000000UL : 50000000UL;
	if (freq > max_freq) freq = max_freq;
	int div = (LPC_ADC_BASE_FREQ + (freq - 1)) / freq;
	adc->CTRL = ((div - 1) << ADC_CTRL_CLKDIV_BIT) |
		(flags & LPC_ADC_INITF_ASYNC ? ADC_CTRL_ASYNMODE : 0) |
		(flags & LPC_ADC_INITF_FAST ? ADC_CTRL_MODE10BIT : 0) |
		(flags & LPC_ADC_INITF_LOWPOWER ? ADC_CTRL_LPWRMODE : 0);
	
	adc->SEQA_CTRL = adc->SEQB_CTRL = 0;
}

void adc_initialize(int module, int sample_rate, LPC_ADC_INIT_FLAGS flags)
{
	switch(module)
	{
		case 0:
			LPC_SYSCON->PRESETCTRL0 |= PRESETCTRL0_ADC0;
			LPC_SYSCON->SYSAHBCLKCTRL0 |= CLKCTRL0_ADC0;
			LPC_SYSCON->PRESETCTRL0 &= ~PRESETCTRL0_ADC0;
			LPC_SYSCON->PDRUNCFG &= ~PDRUNCFG_ADC0;
			_init(LPC_ADC0, sample_rate, flags);
			NVIC_EnableIRQ(ADC0_THCMP_IRQn);
			NVIC_EnableIRQ(ADC0_OVR_IRQn);
			break;
		case 1:	
			LPC_SYSCON->PRESETCTRL1 |= PRESETCTRL0_ADC1;
			LPC_SYSCON->SYSAHBCLKCTRL0 |= CLKCTRL0_ADC1;
			LPC_SYSCON->PRESETCTRL1 &= ~PRESETCTRL0_ADC1;
			LPC_SYSCON->PDRUNCFG &= ~PDRUNCFG_ADC1;
			_init(LPC_ADC1, sample_rate, flags);
			NVIC_EnableIRQ(ADC1_THCMP_IRQn);
			NVIC_EnableIRQ(ADC1_OVR_IRQn);
			break;
	}
}

void adc_config_seq_irqmode(int module, int seq, LPC_ADC_SEQ_CONFIG *conf, LPC_ADC_SEQ_HANDLER handler)
{
    LPC_ADC0_Type *adc;
	int irqn;
	switch(module)
	{
		case 0: 
			adc = LPC_ADC0;
			irqn = (seq == 0) ? ADC0_SEQA_IRQn : ADC0_SEQB_IRQn;
			break;
		case 1:	
			adc = LPC_ADC1;
			irqn = (seq == 0) ? ADC1_SEQA_IRQn : ADC1_SEQB_IRQn;
			break;
		default:	return;
	}

	unsigned int value = ADC_SEQCTRL_MODE | ADC_SEQCTRL_ENA |
			((conf->Channels & 0xFFF) << ADC_SEQCTRL_CHANNELS_BIT) |
			(conf->Trigger == 0 ? ADC_SEQCTRL_START :
				( ((conf->Trigger & 0xF) << ADC_SEQCTRL_TRIGGER_BIT) | 
				( conf->Polarity == 0 ? ADC_SEQCTRL_TRIGPOL : 0))) |
			(conf->Flags & LPC_ADC_SEQF_SINGLESTEP ? ADC_SEQCTRL_SINGLESTEP : 0) |
			(conf->Flags & LPC_ADC_SEQF_LOW_PRI ? ADC_SEQCTRL_LOWPRIO : 0);
	if (seq == 0)
	{
		adc->INTEN |= ADC_INTEN_SEQA;
		adc->SEQA_CTRL = value;
	}
	else
	{
		adc->INTEN |= ADC_INTEN_SEQB;
		adc->SEQB_CTRL = value;
	}

	NVIC_EnableIRQ(irqn);
}

int adc_read(int module, int ch, unsigned short *pval)
{
    LPC_ADC0_Type *adc;
	switch(module)
	{
		case 0: adc = LPC_ADC0; break;
		case 1:	adc = LPC_ADC1; break;
		default:	return 0;
	}

	unsigned long reg = adc->DAT[ch];
	if (reg & 0x80000000)
	{
		*pval = ((reg & 0xFFF0) * 3300) >> 16;	// NOTE: mV; make Vref adjustable
		return 1;
	}
	return  0;
}

static void _disable_thr_inten(LPC_ADC0_Type *adc, int channel)
{
	switch(channel)
	{
		case 0: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 1: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 2: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 3: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 4: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 5: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 6: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 7: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 8: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 9: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 10: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
		case 11: adc->INTEN &= ~(3 << ADC_INTEN_ADCMP0_BIT);	break;
	}
}

void adc_config_thr(int module, int sel, unsigned int ch_mask, LPC_ADC_THRESHOLD *thr, LPC_ADC_THR_FLAGS flags)
{
    LPC_ADC0_Type *adc;
	switch(module)
	{
		case 0: adc = LPC_ADC0; break;
		case 1:	adc = LPC_ADC1; break;
		default:	return;
	}

	unsigned int curr_mask = sel ? adc->CHAN_THRSEL : ~adc->CHAN_THRSEL;
	for(int ch = 0; ch < 12; ch++)
	{
		if ((curr_mask | ch_mask) & (1<<ch))
			adc->INTEN &= (3 << (ADC_INTEN_ADCMP0_BIT + (ch << 1)));
	}

	switch(sel)
	{
		case 0:
			adc->THR0_LOW = thr->Low;
			adc->THR0_HIGH = thr->High;
			adc->CHAN_THRSEL &= ~ch_mask;
			break;
		case 1:
			adc->THR1_LOW = thr->Low;
			adc->THR1_HIGH = thr->High;
			adc->CHAN_THRSEL |= ch_mask;
			break;
	}

	ADC_INTEN_CMP cmp = (flags & LPC_ADC_THRF_OUTSIDE_IRQ) ?  ADC_INTEN_CMP_OUTSIDE :
		((flags & LPC_ADC_THRF_CROSSING_IRQ) ? ADC_INTEN_CMP_CROSSING : ADC_INTEN_CMP_DISABLE);
	for(int ch = 0; ch < 12; ch++)
	{
		if (ch_mask & (1<<ch))
			adc->INTEN |= (cmp << (ADC_INTEN_ADCMP0_BIT + (ch << 1)));
	}
}

static void _halt()
{
}

void ADC0_SEQA_IRQHandler()
{
	LPC_ADC0->FLAGS = ADC_FLAGS_SEQA_INT;
}

void ADC0_SEQB_IRQHandler()
{
	LPC_ADC0->FLAGS = ADC_FLAGS_SEQB_INT;
}

void ADC0_THCMP_IRQHandler()
{
	_halt();
}

void ADC0_OVR_IRQHandler()
{
	_halt();
}

void ADC1_SEQA_IRQHandler()
{
	LPC_ADC1->FLAGS = ADC_FLAGS_SEQA_INT;
}

void ADC1_SEQB_IRQHandler()
{
	LPC_ADC1->FLAGS = ADC_FLAGS_SEQB_INT;
}

void ADC1_THCMP_IRQHandler()
{
	_halt();
}

void ADC1_OVR_IRQHandler()
{
	_halt();
}

