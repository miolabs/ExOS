// ExOS CPU Dependant Code (ARMv4)
// for use with ARM7T or ARM9T cores
// by Miguel Fides
 
	.section ".text", "ax"
	.code 32
	.global SWI_Handler
	.global __switch
	.global __kernel_do

__kernel_do:
	stmfd sp!, {r1-r3}		// push argument to stack
	mov r1, r0
	mov r0, sp
	stmfd sp!, {lr}
	swi #0
	ldmfd sp!, {lr}
	add sp, #12
	bx lr

SWI_Handler:	// r0:args r1:dispatcher
	mrs r2, spsr
	and r3, r2, #0x1F
	cmp r3, #0x10	// user mode
	beq 1f
	cmp r3, #0x1f	// system mode
	beq 1f
	cmp r3, #0x12	// IRQ mode
	bne 2f

	// IRQ mode call
	stmfd sp!, {r2, lr}
	mov lr, pc
	bx r1
	ldmfd sp!, {r2, lr}
	msr spsr, r2	// restore spsr
	movs pc, lr

2:	stmfd sp!, {lr}
	mov r0, #0
	bl	kernel_panic
	ldmfd sp!, {lr}
	movs pc, lr
	
	// User/System mode call
1:	stmfd sp!, {lr}
	mov lr, pc
	bx r1	// call dispatcher
	ldmfd sp!, {lr}	

__switch:
	stmfd sp!, {r0-r3, lr}
	ldr r1, =__pend_switch	// check pending switch
	ldrb r2, [r1]
	cmp r2, #0
	beq 1f

	mov r2, #0
	strb r2, [r1]	// clear pending switch

	ldr r1, =__psp
	stm r1, {sp}^
	ldr r2, [r1]	// get thread sp

	mrs r3, spsr
	sub r2, #16
	ldr r0, [sp, #16]	//pc
	stm r2, {r0, r3, r12, lr}^

	sub r2, #32
	stm r2, {r4-r11}
	ldm sp, {r4-r7}		// r0-r3
	sub r2, #16
	stm r2, {r4-r7}
	str r2, [r1]

	bl __machine_switch

	ldr r1, =__psp
	ldr r2, [r1]
	ldmfd r2!, {r4-r7}
	stm sp, {r4-r7}
	ldmfd r2!, {r4-r11}
	ldmfd r2, {r0, r3, r12, lr}^
	add r2, #16
	str r0, [sp, #16]
	str r2, [r1]

	ldm r1, {sp}^	// set thread sp
	msr spsr, r3

1:	ldmfd sp!, {r0-r3, lr}
	movs pc, lr




