#include <kernel/machine/hal.h>
#include <support/stm32f4/cpu.h>
//#include <kernel/time.h>
#include <kernel/memory.h>
#include <kernel/panic.h>

//NOTE: stm32f4 can dma to all memory, so heap is dma-able
EXOS_MEM_FLAGS __machine_heap_flags = EXOS_MEMF_DMA;

#define NVIC_VECTORS 91

void __machine_init()
{
	NVIC_SetPriority(PendSV_IRQn, 0xFF);	// set lowest priority for PendSV

	for (int i = 0; i <= NVIC_VECTORS; i++)
	{
		NVIC_DisableIRQ((IRQn_Type)i);
		NVIC_SetPriority((IRQn_Type)i, 0xFF);		// set lowest priority for IRQ
	}

	bool pll_config = false;
#ifdef HSE_VALUE
	if (!(RCC->CR & RCC_CR_HSERDY))
	{
		RCC->CR |= RCC_CR_HSEON;
		while(!(RCC->CR & RCC_CR_HSERDY));
		pll_config = true;
	}
	#define PLLIN_VALUE HSE_VALUE
#else
	#define PLLIN_VALUE 16000000UL // NOTE: HSI internal RC clock
#endif

	if (pll_config)
	{
#ifdef PLLM_VALUE
		ASSERT(PLLM_VALUE >= 2 && PLLM_VALUE <= 63, KERNEL_ERROR_KERNEL_PANIC);
		ASSERT(PLLN_VALUE >= 192 && PLLN_VALUE <= 432, KERNEL_ERROR_KERNEL_PANIC);
		ASSERT(PLLP_VALUE >= 2 && PLLP_VALUE <= 8 && (PLLP_VALUE & 1) == 0, KERNEL_ERROR_KERNEL_PANIC);
		ASSERT(PLLQ_VALUE >= 2 && PLLQ_VALUE <= 15, KERNEL_ERROR_KERNEL_PANIC);
		RCC->PLLCFGR = (PLLM_VALUE << RCC_PLLCFGR_PLLM_BIT)
			| (PLLN_VALUE << RCC_PLLCFGR_PLLN_BIT)
			| (((PLLP_VALUE >> 1) - 1) << RCC_PLLCFGR_PLLP_BIT)
#ifdef HSE_VALUE
			| (RCC_PLLCFGR_PLLSRC)				// select HSE source for PLL
#endif
			| (PLLQ_VALUE << RCC_PLLCFGR_PLLQ_BIT);

		unsigned pll_in = PLLIN_VALUE;
		unsigned vco_in = pll_in / PLLM_VALUE;
		ASSERT(vco_in >= 1000000UL && vco_in <= 2000000UL, KERNEL_ERROR_KERNEL_PANIC);
		unsigned vco_out = vco_in * PLLN_VALUE;
		unsigned main_clk = vco_out / PLLP_VALUE;
		ASSERT(main_clk <= 168000000UL, KERNEL_ERROR_KERNEL_PANIC);
		unsigned usb_clk = vco_out / PLLQ_VALUE;
		ASSERT(usb_clk <= 48000000UL, KERNEL_ERROR_KERNEL_PANIC);

		//NOTE: latency selection for STM32F405xx/07xx and STM32F415xx/17xx
		unsigned latency = 0;
		if (main_clk > 150000000UL) latency = 5;
		else if (main_clk > 120000000UL) latency = 4;
		else if (main_clk > 90000000UL) latency = 3;
		else if (main_clk > 60000000UL) latency = 2;
		else if (main_clk > 30000000UL) latency = 1;

		FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | latency;
		while((FLASH->ACR & FLASH_ACR_LATENCY) != latency);

		if (latency != 0)
			FLASH->ACR |= FLASH_ACR_PRFTEN;	// enable flash prefetch buffer

		unsigned ppre1 = 0;
		for(unsigned apb1 = main_clk; apb1 >= 42000000UL; apb1 >>= 1) ppre1++;
		unsigned ppre2 = 0;
		for(unsigned apb2 = main_clk; apb2 >= 84000000UL; apb2 >>= 1) ppre2++;
	
		RCC->CFGR = (RCC->CFGR & RCC_CFGR_SW)
			| (((ppre1 == 0) ? 0 : (4 | (ppre1 - 1)))  << RCC_CFGR_PPRE1_BIT)
			| (((ppre2 == 0) ? 0 : (4 | (ppre2 - 1))) << RCC_CFGR_PPRE2_BIT)
			| (0 << RCC_CFGR_HPRE_BIT);

		RCC->CR |= RCC_CR_PLLON;
		while(!(RCC->CR & RCC_CR_PLLRDY));
		RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;	// select PLL source for SYSCLK
		while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
#elif defined HSE_VALUE
		RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSE;	// select HSE source for SYSCLK
		while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);
#endif
	}

	SystemCoreClockUpdate();
	
#ifdef TARGET_SYSTEM_CLK
	if (SystemCoreClock != TARGET_SYSTEM_CLK)
		kernel_panic(KERNEL_ERROR_KERNEL_PANIC);
#endif

	NVIC_SetPriority(SysTick_IRQn, 0xFF);	// set Priority for Systick Interrupt
	SysTick->LOAD = (SystemCoreClock / 1000) - 1;
	SysTick->CTRL = 0;

#ifdef KERNEL_DMA_MEMORY	
	mem_init_region(_kernel_mem, KERNEL_DMA_MEMORY, MEMF_DMA, -1);
#endif

	__board_init();
}

void __machine_req_switch()
{
	SCB->ICSR = (1 << 28);	// Set pending PendSV service (switch)
}

void __machine_reset()
{
#ifdef DEBUG
	__BKPT(0);
#endif
	NVIC_SystemReset();
	while(1);
}

void __machine_reboot(void **vbase)
{
	__disable_irq();
	SysTick->CTRL = 0;

	for(unsigned i = 0; i <= NVIC_VECTORS; i++)
	{
		NVIC_DisableIRQ((IRQn_Type)i);

	}
	SCB->VTOR = ((unsigned)vbase & 0xFFFFFF) + FLASH_BASE;
	__enable_irq();

	__asm__ volatile (
		"mov r0, %0\n\t"
		"ldr r1, [r0, #0]\n\t"
		"msr msp, r1\n\t"		// load sp from vector #0
		"mov r1, #0\n\t"
		"msr control, r1\n\t"	// switch to system stack
		"ldr r1, [r0, #4]\n\t"	// load reset vector
		"bx r1\n\t"
		: : "r" (vbase));

	// you should never get here
	__machine_reset();
}

void __machine_halt()
{
#ifdef DEBUG
	__BKPT(0);
#endif
	NVIC_SystemReset();
	while(1);
}

void __machine_idle(void *args)
{
#ifdef DEBUG	
	while(1);
#else
	while(1)
	{
//		__WFI();
	}
#endif
}


