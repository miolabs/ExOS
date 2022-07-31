#include "cpu.h"
#include <kernel/panic.h>

#define HSI_VALUE    ((uint32_t)16000000) 

// NOTE: SystemCoreClock = HCLK

unsigned cpu_get_pclk1()
{
	unsigned ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_BIT;
	unsigned pclk1 = ppre1 == 0 ? SystemCoreClock :
		(SystemCoreClock >> ((ppre1 & 0x3) + 1));
	return pclk1;
}

unsigned cpu_get_pclk2()
{
	unsigned ppre2 = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_BIT;
	unsigned pclk2 = ppre2 == 0 ? SystemCoreClock :
		(SystemCoreClock >> ((ppre2 & 0x3) + 1));
	return pclk2;
}

unsigned cpu_get_pll_input()
{
	unsigned pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
	unsigned pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
	unsigned vcin = ((pllsource != 0) ? HSE_VALUE : HSI_VALUE) / pllm;
	return vcin;
}

unsigned cpu_setup_plli2s(unsigned n, unsigned q, unsigned r)
{
	unsigned vcin = cpu_get_pll_input();	// NOTE: already divided by m
	ASSERT(n >= 192 && n <= 432, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(q >= 2 && q <= 15, KERNEL_ERROR_KERNEL_PANIC);
	ASSERT(r >= 2 && r <= 7, KERNEL_ERROR_KERNEL_PANIC);
	RCC->PLLI2SCFGR = (n << RCC_PLLI2SCFGR_PLLN_BIT)
		| (q << RCC_PLLI2SCFGR_PLLQ_BIT)
		| (r << RCC_PLLI2SCFGR_PLLR_BIT);
	unsigned vco = (vcin * n);
	return vco;
}

void cpu_setup_mco2(mco2_source_t src, unsigned div)
{
	ASSERT(div != 0 && div <= 5, KERNEL_ERROR_KERNEL_PANIC);
	unsigned value = (src << RCC_CFGR_MCO2_BIT) | 
		((div == 1 ? 0 : (0x4 | (div - 2))) << RCC_CFGR_MCO2PRE_BIT);
	unsigned mask = (0x3 << RCC_CFGR_MCO2_BIT) |
		(0x7 << RCC_CFGR_MCO2PRE_BIT);
	RCC->CFGR = (RCC->CFGR & ~mask) | (value & mask);  
}

unsigned cpu_get_sdioclk()
{
	return cpu_get_pll_qclk();
}

unsigned cpu_get_pll_qclk()
{
	unsigned vcin = cpu_get_pll_input();	// NOTE: already divided by m
	unsigned n = (RCC->PLLCFGR & RCC_PLLCFGR_PLLN_Msk) >> RCC_PLLCFGR_PLLN_BIT;
    ASSERT(n >= 192 && n <= 432, KERNEL_ERROR_KERNEL_PANIC);
	unsigned vco = vcin * n;
	unsigned q = (RCC->PLLCFGR & RCC_PLLCFGR_PLLQ_Msk) >> RCC_PLLCFGR_PLLQ_BIT;
    ASSERT(q >= 2 && q <= 15, KERNEL_ERROR_KERNEL_PANIC);
	return vco / q;
}


