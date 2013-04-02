	.section ".init", "ax"
	.code 32
	.global Reset_Handler

#define VIC_BASE 0xFFFFF000
#define VICADDR 0xF00
#define VICVectAddr0 0x100

#define SCB_BASE 0xE01FC000
#define MAMCR 0x0
#define MAMTIM 0x4
#define PLLCON 0x80
#define PLLCON_PLLE 0x1
#define PLLCON_PLLC 0x2
#define PLLCFG 0x84
#define PLLCFG_MSEL_BIT 0
#define PLLCFG_NSEL_BIT 16
#define PLLSTAT 0x88
#define PLLSTAT_PLLC 0x2000000
#define PLLSTAT_PLLC_BIT 25
#define PLLSTAT_PLOCK 0x4000000
#define PLLSTAT_PLOCK_BIT 26
#define PLLFEED 0x8C
#define CCLKCFG 0x104
#define USBCLKCFG 0x108
#define CLKSRCSEL 0x10C
#define CLKSRCSEL_CLKSRC_BIT 0
#define SCS 0x1A0
#define SCS_OSCEN 0x20
#define SCS_OSCEN_BIT 5
#define SCS_OSCSTAT 0x40
#define SCS_OSCSTAT_BIT 6

#if OSCILLATOR_CLOCK_FREQUENCY==12000000

/* Fosc = 12Mhz, Fcco = 288Mhz */
#ifndef PLLCFG_VAL
#define PLLCFG_VAL ((11 << PLLCFG_MSEL_BIT) | (0 << PLLCFG_NSEL_BIT))
#endif
#ifndef CCLKCFG_VAL
/* 5 = cclk 48MHz, 3 = cclk 72MHz */
#define CCLKCFG_VAL 3 
#endif
#ifndef USBCLKCFG_VAL
#define USBCLKCFG_VAL 5
#endif

#endif

#ifndef MAMCR_VAL
#define MAMCR_VAL 2
#endif

#ifndef MAMTIM_VAL
#define MAMTIM_VAL 3
#endif

Reset_Handler:
	ldr r0, =SCB_BASE

#if defined(PLLCFG_VAL) && !defined(NO_PLL_ENABLE)
	/* Configure PLL Multiplier/Divider */
	ldr r1, [r0, #PLLSTAT]
	tst r1, #PLLSTAT_PLLC
	beq 1f

	/* Disconnect PLL */
	ldr r1, =PLLCON_PLLE
	str r1, [r0, #PLLCON]
	mov r1, #0xAA
	str r1, [r0, #PLLFEED]
	mov r1, #0x55
	str r1, [r0, #PLLFEED]
1:
	/* Disable PLL */
	ldr r1, =0
	str r1, [r0, #PLLCON]
	mov r1, #0xAA
	str r1, [r0, #PLLFEED]
	mov r1, #0x55
	str r1, [r0, #PLLFEED]

	/* Enable main oscillator */
	ldr r1, [r0, #SCS]
	orr r1, r1, #SCS_OSCEN
	str r1, [r0, #SCS]
1:	ldr r1, [r0, #SCS]
	tst r1, #SCS_OSCSTAT
	beq 1b

	/* Select main oscillator as the PLL clock source */
	ldr r1, =1
	str r1, [r0, #CLKSRCSEL]
	
	/* Set PLLCFG */
	ldr r1, =PLLCFG_VAL
	str r1, [r0, #PLLCFG]
	mov r1, #0xAA
	str r1, [r0, #PLLFEED]
	mov r1, #0x55
	str r1, [r0, #PLLFEED]
	
	/* Enable PLL */
	ldr r1, =PLLCON_PLLE
	str r1, [r0, #PLLCON]
	mov r1, #0xAA
	str r1, [r0, #PLLFEED]
	mov r1, #0x55
	str r1, [r0, #PLLFEED]

#ifdef CCLKCFG_VAL
	/* Set the CPU clock divider */
	ldr r1, =CCLKCFG_VAL
	str r1, [r0, #CCLKCFG]
#endif

#ifdef USBCLKCFG_VAL
	/* Set the CPU clock divider */
	ldr r1, =USBCLKCFG_VAL
	str r1, [r0, #USBCLKCFG]
#endif
  
  /* Wait for PLL to lock */
1:	ldr r1, [r0, #PLLSTAT]
	tst r1, #PLLSTAT_PLOCK
	beq 1b
	/* PLL Locked, connect PLL as clock source */
	mov r1, #(PLLCON_PLLE | PLLCON_PLLC)
	str r1, [r0, #PLLCON]
	mov r1, #0xAA
	str r1, [r0, #PLLFEED]
	mov r1, #0x55
	str r1, [r0, #PLLFEED]
	/* Wait for PLL to connect */
1:	ldr r1, [r0, #PLLSTAT]
	tst r1, #PLLSTAT_PLLC
	beq 1b
#endif

	/* Initialise memory accelerator module */
	mov r1, #0
	str r1, [r0, #MAMCR]
	ldr r1, =MAMTIM_VAL
	str r1, [r0, #MAMTIM]
	ldr r1, =MAMCR_VAL
	str r1, [r0, #MAMCR]

	// setup stacks
	mrs r4, cpsr
	bic r4, r4, #0x1F

	orr r3, r4, #0x12	// IRQ mode
	msr cpsr_cxsf, r3
	ldr sp, =__stack_irq_end__
	ldr r0, =__stack_irq_start__
	bl init_stack

	orr r3, r4, #0x13	// Supervisor mode
	msr cpsr_cxsf, r3
    ldr sp, =__stack_svc_end__
	ldr r0, =__stack_svc_start__
	bl init_stack

	orr r3, r4, #0x1F	// System mode
	msr cpsr_cxsf, r3
	ldr sp, =__stack_end__
	ldr r0, =__stack_start__
	bl init_stack

	ldr r3, =IRQTable
	ldr r2, =(VIC_BASE + VICVectAddr0)
	mov r1, #32
1:	ldr r0, [r3], #4
	str r0, [r2], #4
	subs r1, r1, #1
	bne 1b

	ldr pc, =__kernel_start


init_stack:
	ldr r1, =0xcccccccc
	mov r2, sp
	cmp r2, r1
	movle pc, lr
1:	str r1, [r2, #-4]!
	cmp r2, r0
	bhi 1b
	mov pc, lr

	.section ".text", "ax"
	.code 32
	.global IRQ_Handler


IRQ_Handler:
	sub lr, lr, #4	// correct link address for irq
	stmfd sp!, {r0-r3, lr}

	// get irq handler
	ldr r3, =VIC_BASE
	ldr r2, [r3]
	ldr r2, [r3, #VICADDR]

	// call interrupt handler
	stmfd sp!, {r12}
	mov lr, pc
	bx r2
	ldmfd sp!, {r12}

	ldr r3, =VIC_BASE
	mov r1, #0
	str r1, [r3, #VICADDR]

	ldmfd sp!, {r0-r3, lr}

	b __switch


.macro DEFAULT_ISR_HANDLER name=
  .weak \name
\name:
1:	b 1b /* endless loop */
.endm

			.align 8
IRQTable:	.word 0
			.word 0
			.word 0
			.word 0
			.word TIMER0_IRQHandler
			.word TIMER1_IRQHandler  
			.word UART0_IRQHandler   
			.word UART1_IRQHandler   
			.word PWM_IRQHandler    
			.word I2C0_IRQHandler       
			.word SSP0_IRQHandler       
			.word SSP1_IRQHandler       
			.word PLL0_IRQHandler    
			.word RTC_IRQHandler     
			.word EINT0_IRQHandler   
			.word EINT1_IRQHandler   
			.word EINT2_IRQHandler   
			.word EINT3_IRQHandler   
			.word ADC_IRQHandler     
			.word I2C1_IRQHandler       
			.word BOD_IRQHandler     
			.word ENET_IRQHandler    
			.word USB_IRQHandler     
			.word CAN_IRQHandler     
			.word MCI_IRQHandler
			.word DMA_IRQHandler     
			.word TIMER2_IRQHandler  
			.word TIMER3_IRQHandler  
			.word UART2_IRQHandler   
			.word UART3_IRQHandler   
			.word I2C2_IRQHandler       
			.word I2S_IRQHandler

DEFAULT_ISR_HANDLER WDT_IRQHandler
DEFAULT_ISR_HANDLER SoftInt_IRQHandler
DEFAULT_ISR_HANDLER DbgCommRx_IRQHandler
DEFAULT_ISR_HANDLER DbgCommTx_IRQHandler
DEFAULT_ISR_HANDLER TIMER0_IRQHandler
DEFAULT_ISR_HANDLER TIMER1_IRQHandler  
DEFAULT_ISR_HANDLER UART0_IRQHandler   
DEFAULT_ISR_HANDLER UART1_IRQHandler   
DEFAULT_ISR_HANDLER PWM_IRQHandler    
DEFAULT_ISR_HANDLER I2C0_IRQHandler       
DEFAULT_ISR_HANDLER SSP0_IRQHandler       
DEFAULT_ISR_HANDLER SSP1_IRQHandler       
DEFAULT_ISR_HANDLER PLL0_IRQHandler    
DEFAULT_ISR_HANDLER RTC_IRQHandler     
DEFAULT_ISR_HANDLER EINT0_IRQHandler   
DEFAULT_ISR_HANDLER EINT1_IRQHandler   
DEFAULT_ISR_HANDLER EINT2_IRQHandler   
DEFAULT_ISR_HANDLER EINT3_IRQHandler   
DEFAULT_ISR_HANDLER ADC_IRQHandler     
DEFAULT_ISR_HANDLER I2C1_IRQHandler       
DEFAULT_ISR_HANDLER BOD_IRQHandler     
DEFAULT_ISR_HANDLER ENET_IRQHandler    
DEFAULT_ISR_HANDLER USB_IRQHandler     
DEFAULT_ISR_HANDLER CAN_IRQHandler     
DEFAULT_ISR_HANDLER MCI_IRQHandler
DEFAULT_ISR_HANDLER DMA_IRQHandler     
DEFAULT_ISR_HANDLER TIMER2_IRQHandler  
DEFAULT_ISR_HANDLER TIMER3_IRQHandler  
DEFAULT_ISR_HANDLER UART2_IRQHandler   
DEFAULT_ISR_HANDLER UART3_IRQHandler   
DEFAULT_ISR_HANDLER I2C2_IRQHandler       
DEFAULT_ISR_HANDLER I2S_IRQHandler


