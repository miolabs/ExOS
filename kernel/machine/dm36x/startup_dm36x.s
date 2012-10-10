	.section ".init", "ax"
	.code 32
	.global Reset_Handler

#define INTC_BASE 0x01C48000
#define EABASE_OFFSET 0x024
#define IRQ0_OFFSET 0x008
#define IRQ1_OFFSET 0x00C
#define IRQENTRY_OFFSET 0x014

Reset_Handler:
#ifndef DONT_ENABLE_ICACHE
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0	// invalide icache
	ldr r1, [r0]

	// enable caches
	mrc p15, 0, r2, c1, c0, 0	// read control register
	orr r2, #(1<<12)	// I (enable icache)
	mcr p15, 0, r2, c1, c0, 0	// write control register
#endif

#ifdef REENABLE_ITCM
	// reenable ITCM
	ldr r2, =(0x00000000 | (6 << 2) | 1);
	mcr p15, 0, r2, c9, c1, 1
	
	str r1, [r0]
	b 1f
1:	mcr p15, 0, r0, c7, c10, 4 // drain write buffer
#endif

	// setup IRQ interrupt table for INTC
	ldr r2, =INTC_BASE 
	ldr r1, =IRQTable
	str r1, [r2, #EABASE_OFFSET]
	ldr r1, =0xffffffff
	str r1, [r2, #IRQ0_OFFSET]
	str r1, [r2, #IRQ1_OFFSET]

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

	// get current irq handler
	ldr r3, =INTC_BASE
	ldr r2, [r3, #IRQENTRY_OFFSET]
	ldr r1, =(IRQTable + 4)
	sub r1, r2, r1
	lsr r1, #2

	// clear interrupt
	mov r0, #1
	tst r1, #32
	and r1, r1, #31
	lsl r0, r0, r1
	streq r0, [r3, #IRQ0_OFFSET]
	strne r0, [r3, #IRQ1_OFFSET]

	// call interrupt handler
	ldr r0, [r2]
	mov lr, pc
	bx r0
	ldmfd sp!, {r0-r3, lr}

	b __switch




.macro DEFAULT_ISR_HANDLER name=
  .weak \name
\name:
1:	b 1b /* endless loop */
.endm

			.align 8
IRQTable:	.word 0
			.word INT00_Handler
			.word INT01_Handler
			.word INT02_Handler
			.word INT03_Handler
			.word INT04_Handler
			.word INT05_Handler
			.word INT06_Handler
			.word INT07_Handler
			.word INT08_Handler
			.word INT09_Handler
			.word INT10_Handler
			.word INT11_Handler
			.word INT12_Handler
			.word INT13_Handler
			.word INT14_Handler
			.word INT15_Handler
			.word INT16_Handler
			.word INT17_Handler
			.word INT18_Handler
			.word INT19_Handler
			.word INT20_Handler
			.word INT21_Handler
			.word INT22_Handler
			.word INT23_Handler
			.word INT24_Handler
			.word INT25_Handler
			.word INT26_Handler
			.word INT27_Handler
			.word INT28_Handler
			.word INT29_Handler
			.word INT30_Handler
			.word INT31_Handler
			.word INT32_Handler
			.word INT33_Handler
			.word INT34_Handler
			.word INT35_Handler
			.word INT36_Handler
			.word INT37_Handler
			.word INT38_Handler
			.word INT39_Handler
			.word INT40_Handler
			.word INT41_Handler
			.word INT42_Handler
			.word INT43_Handler
			.word INT44_Handler
			.word INT45_Handler
			.word INT46_Handler
			.word INT47_Handler
			.word INT48_Handler
			.word INT49_Handler
			.word INT50_Handler
			.word INT51_Handler
			.word INT52_Handler
			.word INT53_Handler
			.word INT54_Handler
			.word INT55_Handler
			.word INT56_Handler
			.word INT57_Handler
			.word INT58_Handler
			.word INT59_Handler
			.word INT60_Handler
			.word INT61_Handler
			.word INT62_Handler
			.word INT63_Handler

DEFAULT_ISR_HANDLER INT00_Handler
DEFAULT_ISR_HANDLER INT01_Handler
DEFAULT_ISR_HANDLER INT02_Handler
DEFAULT_ISR_HANDLER INT03_Handler
DEFAULT_ISR_HANDLER INT04_Handler
DEFAULT_ISR_HANDLER INT05_Handler
DEFAULT_ISR_HANDLER INT06_Handler
DEFAULT_ISR_HANDLER INT07_Handler
DEFAULT_ISR_HANDLER INT08_Handler
DEFAULT_ISR_HANDLER INT09_Handler
DEFAULT_ISR_HANDLER INT10_Handler
DEFAULT_ISR_HANDLER INT11_Handler
DEFAULT_ISR_HANDLER INT12_Handler
DEFAULT_ISR_HANDLER INT13_Handler
DEFAULT_ISR_HANDLER INT14_Handler
DEFAULT_ISR_HANDLER INT15_Handler
DEFAULT_ISR_HANDLER INT16_Handler
DEFAULT_ISR_HANDLER INT17_Handler
DEFAULT_ISR_HANDLER INT18_Handler
DEFAULT_ISR_HANDLER INT19_Handler
DEFAULT_ISR_HANDLER INT20_Handler
DEFAULT_ISR_HANDLER INT21_Handler
DEFAULT_ISR_HANDLER INT22_Handler
DEFAULT_ISR_HANDLER INT23_Handler
DEFAULT_ISR_HANDLER INT24_Handler
DEFAULT_ISR_HANDLER INT25_Handler
DEFAULT_ISR_HANDLER INT26_Handler
DEFAULT_ISR_HANDLER INT27_Handler
DEFAULT_ISR_HANDLER INT28_Handler
DEFAULT_ISR_HANDLER INT29_Handler
DEFAULT_ISR_HANDLER INT30_Handler
DEFAULT_ISR_HANDLER INT31_Handler
DEFAULT_ISR_HANDLER INT32_Handler
DEFAULT_ISR_HANDLER INT33_Handler
DEFAULT_ISR_HANDLER INT34_Handler
DEFAULT_ISR_HANDLER INT35_Handler
DEFAULT_ISR_HANDLER INT36_Handler
DEFAULT_ISR_HANDLER INT37_Handler
DEFAULT_ISR_HANDLER INT38_Handler
DEFAULT_ISR_HANDLER INT39_Handler
DEFAULT_ISR_HANDLER INT40_Handler
DEFAULT_ISR_HANDLER INT41_Handler
DEFAULT_ISR_HANDLER INT42_Handler
DEFAULT_ISR_HANDLER INT43_Handler
DEFAULT_ISR_HANDLER INT44_Handler
DEFAULT_ISR_HANDLER INT45_Handler
DEFAULT_ISR_HANDLER INT46_Handler
DEFAULT_ISR_HANDLER INT47_Handler
DEFAULT_ISR_HANDLER INT48_Handler
DEFAULT_ISR_HANDLER INT49_Handler
DEFAULT_ISR_HANDLER INT50_Handler
DEFAULT_ISR_HANDLER INT51_Handler
DEFAULT_ISR_HANDLER INT52_Handler
DEFAULT_ISR_HANDLER INT53_Handler
DEFAULT_ISR_HANDLER INT54_Handler
DEFAULT_ISR_HANDLER INT55_Handler
DEFAULT_ISR_HANDLER INT56_Handler
DEFAULT_ISR_HANDLER INT57_Handler
DEFAULT_ISR_HANDLER INT58_Handler
DEFAULT_ISR_HANDLER INT59_Handler
DEFAULT_ISR_HANDLER INT60_Handler
DEFAULT_ISR_HANDLER INT61_Handler
DEFAULT_ISR_HANDLER INT62_Handler
DEFAULT_ISR_HANDLER INT63_Handler


