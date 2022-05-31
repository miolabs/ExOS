// IRQ handler for INTC (TI TMS320DM36x)
//
// part of the KIO Kernel
// Copyright (c) 2010, 2011 Miguel Fides and Joseluis Cortes
// All right reserved


#define INTC_BASE 0x01C48000
#define IRQ0_OFFSET 0x008
#define IRQ1_OFFSET 0x00C
#define IRQENTRY_OFFSET 0x014

	.code 32
	.global irq_handler

irq_handler:
	sub lr, lr, #4	// correct link address for irq
	
	// check if I bit is set in SPSR to detect interrupt occuring during interrupt disable
	stmfd sp!, {r0}
	mrs r0, spsr
	tst r0, #0x80
	ldmfd sp!, {r0}
	movnes pc, lr

	stmfd sp!, {r0-r4, lr}

	// get current irq handler
	ldr r4, =INTC_BASE
	ldr r5, [r4, #IRQENTRY_OFFSET]

	// clear interrupt
	ldr r1, [r5, #4]
	mov r0, #1
	and r2, r1, #31
	lsl r0, r0, r2
	tst r1, #32
	streq r0, [r4, #IRQ0_OFFSET]
	strne r0, [r4, #IRQ1_OFFSET]

	// call interrupt handler
	ldr r0, [r5, #0]
	mov lr, pc
	bx r0

	// TODO: check pending switch
	bl __switch // context switch

	// return from interrupt
	ldmfd sp!, {r0-r4, lr}
	movs pc, lr