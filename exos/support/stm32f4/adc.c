#include "adc.h"
#include "cpu.h"
#include "dma.h"
#include <support/adc_hal.h>
#include <kernel/panic.h>

#ifndef ADC_MAX_CLOCK
#define ADC_MAX_CLOCK 8000000
#endif

#define ADC_DMA_MODULE 1
#define ADC_MODULE_COUNT 3

static const unsigned _dma_stream_msk[] = { 
	DMA_CHF_0 | DMA_CHF_4, 
	DMA_CHF_2 | DMA_CHF_3, 
	DMA_CHF_0 | DMA_CHF_1 };

static bool _common_configured;
static unsigned _dma_si[ADC_MODULE_COUNT];
static unsigned char _ch_map[ADC_MODULE_COUNT][32];
static unsigned short _buffer[ADC_MODULE_COUNT][32];
static event_t _event[ADC_MODULE_COUNT];

bool adc_initialize(unsigned module, unsigned rate, unsigned bits)
{
	ADC_TypeDef *adc;

	switch(module)
	{
		case 0:
			RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
			adc = ADC1;
			break;
		case 1:		// TODO: ADC2
		case 2:		// TODO: ADC3
			kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
			break;
		default:
			kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
	}

	if (!_common_configured)
	{
		unsigned adcpre = 0;
		unsigned clk = cpu_get_pclk2() >> 1;
		while(clk > ADC_MAX_CLOCK)
		{
			adcpre++;
			clk >>= 1;
		}
		ASSERT(adcpre < 4, KERNEL_ERROR_KERNEL_PANIC);
		ADC->CCR = (adcpre << ADC_CCR_ADCPRE_Pos);	// TODO: DMA mode for multi ADC mode

		NVIC_EnableIRQ(ADC_IRQn);

		_common_configured = true;
	}

	unsigned res;
	switch(bits)
	{
		case 12:	res = 0;	break;
		case 10:	res = 1;	break;
		case 8:		res = 2;	break;
		case 6:		res = 3;	break;
		default:
			kernel_panic(KERNEL_ERROR_NOT_SUPPORTED);
	}
	adc->CR1 = ADC_CR1_EOCIE | ADC_CR1_SCAN | 
		(res << ADC_CR1_RES_Pos);
	adc->CR2 = ADC_CR2_ADON;

	exos_event_create(&_event[module], EXOS_EVENTF_NONE);
	exos_event_set(&_event[module]);	// NOTE: don't wait if conversion not started yet
	if (!dma_alloc_stream(ADC_DMA_MODULE, &_dma_si[module], _dma_stream_msk[module]))	// NOTE: dma stream is never released
			kernel_panic(KERNEL_ERROR_NO_HARDWARE_RESOURCES);
	return true;
} 

static inline ADC_TypeDef *_adc(unsigned module)
{
	switch(module)
	{
		case 0:	return ADC1;
		case 1: return ADC2;
		case 2:	return ADC3;
	}
	kernel_panic(KERNEL_ERROR_NOT_IMPLEMENTED);
}

void adc_restart(unsigned module)
{
	ADC_TypeDef *adc = _adc(module);

	exos_event_reset(&_event[module]);
	adc->CR2 |= ADC_CR2_SWSTART;
}

void adc_start(unsigned module, unsigned ch_mask, adc_mode_t mode)
{
	static const dma_config_t _dma_config[] = {
		{ .Mem = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT, .Increment = 1}, 
			.Periph = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT },
			.Flow = DMA_FLOW_P2M | DMA_FLOW_CIRCULAR | DMA_FLOW_DISABLE_FIFO, .Channel = 0 }, 
		{ .Mem = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT, .Increment = 1}, 
			.Periph = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT },
			.Flow = DMA_FLOW_P2M | DMA_FLOW_CIRCULAR | DMA_FLOW_DISABLE_FIFO, .Channel = 1 }, 
		{ .Mem = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT, .Increment = 1}, 
			.Periph = { .Burst = DMA_BURST_1, .Width = DMA_WIDTH_16BIT },
			.Flow = DMA_FLOW_P2M | DMA_FLOW_CIRCULAR | DMA_FLOW_DISABLE_FIFO, .Channel = 2 } }; 
	
	ADC_TypeDef *adc = _adc(module);
	
	unsigned length = 0;
	for(unsigned ch = 0; ch < 31; ch++)
	{
		if (ch_mask & (1 << ch))
		{
			switch(length)
			{
				case 0:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ1_Msk) | (ch << ADC_SQR3_SQ1_Pos);	break;
				case 1:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ2_Msk) | (ch << ADC_SQR3_SQ2_Pos);	break;
				case 2:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ3_Msk) | (ch << ADC_SQR3_SQ3_Pos);	break;
				case 3:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ4_Msk) | (ch << ADC_SQR3_SQ4_Pos);	break;
				case 4:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ5_Msk) | (ch << ADC_SQR3_SQ5_Pos);	break;
				case 5:		adc->SQR3 = (adc->SQR3 & ~ADC_SQR3_SQ6_Msk) | (ch << ADC_SQR3_SQ6_Pos);	break;
				case 6:		adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ7_Msk) | (ch << ADC_SQR2_SQ7_Pos);	break;
				case 7:		adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ8_Msk) | (ch << ADC_SQR2_SQ8_Pos);	break;
				case 8:		adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ9_Msk) | (ch << ADC_SQR2_SQ9_Pos);	break;
				case 9:		adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ10_Msk) | (ch << ADC_SQR2_SQ10_Pos);	break;
				case 10:	adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ11_Msk) | (ch << ADC_SQR2_SQ11_Pos);	break;
				case 11:	adc->SQR2 = (adc->SQR2 & ~ADC_SQR2_SQ12_Msk) | (ch << ADC_SQR2_SQ12_Pos);	break;
				case 12:	adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ13_Msk) | (ch << ADC_SQR1_SQ13_Pos);	break;
				case 13:	adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ14_Msk) | (ch << ADC_SQR1_SQ14_Pos);	break;
				case 14:	adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ15_Msk) | (ch << ADC_SQR1_SQ15_Pos);	break;
				case 15:	adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_SQ16_Msk) | (ch << ADC_SQR1_SQ16_Pos);	break;
			}

			_ch_map[module][ch] = length++;
		}
	}
	if (length != 0)
	{
		adc->SQR1 = (adc->SQR1 & ~ADC_SQR1_L_Msk) | ((length - 1) << ADC_SQR1_L_Pos);
		adc->CR2 = ADC_CR2_ADON |
			((mode == ADC_MODE_CONTINUOUS) ? ADC_CR2_CONT : 0) | 
			ADC_CR2_DMA | ADC_CR2_DDS | ADC_CR2_ALIGN;

		dma_stream_enable(ADC_DMA_MODULE, _dma_si[module], (void *)&adc->DR, 
			_buffer[module], length, &_dma_config[module], NULL);
		// NOTE: length passed to dma because PeripFlowCTRL not (offically) supported by ADC

		exos_event_reset(&_event[module]);
		adc->CR2 |= ADC_CR2_SWSTART;
	}
}

static void _eoc(unsigned module, ADC_TypeDef *adc)
{
	ASSERT(module < ADC_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	exos_event_set(&_event[module]);
	adc->SR = 0;	// clear STRT flag
}

unsigned short adc_read(unsigned module, unsigned ch)
{
	ASSERT(module < ADC_MODULE_COUNT, KERNEL_ERROR_KERNEL_PANIC);
	unsigned index = _ch_map[module][ch];
	exos_event_wait(&_event[module], EXOS_TIMEOUT_NEVER);
	return _buffer[module][index];
}

void ADC_IRQHandler()
{
	unsigned sr = ADC->CSR;
	if (sr & ADC_CSR_STRT1)
		_eoc(0, ADC1);
	if (sr & ADC_CSR_STRT2)
		_eoc(1, ADC2);
	if (sr & ADC_CSR_STRT3)
		_eoc(2, ADC3);
}


#ifdef ADC_HAL_MODULE

bool hal_adc_initialize(unsigned rate, unsigned bits)
{
	return adc_initialize(ADC_HAL_MODULE, rate, bits);
}

bool hal_adc_read(unsigned channel, unsigned short *presult)
{
	*presult = adc_read(ADC_HAL_MODULE, channel);
	return true;
}

bool hal_adc_start(unsigned ch_mask)
{
	adc_start(ADC_HAL_MODULE, ch_mask, ADC_MODE_ONE_SHOT);	// FIXME: one shot?
	return true;
}

#endif

