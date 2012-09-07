	.section .text


	.global __kernel_do
	.global __mem_copy
	.global __mem_set

	.syntax unified
	.code 16

	.global SVC_Handler
	.thumb_func
SVC_Handler:
	tst lr, #4
	ite eq
	mrseq r2, msp
	mrsne r2, psp

	push {r2, lr}
	ldr r1, [r2]	// dispatcher
	add r0, r2, #4	// arguments
	blx r1			// call dispatcher
	pop {r2}
	
	str	r0, [r2]	// save return value
	pop	{pc}

	.thumb_func
__kernel_do:	// int __kernel_do(KERNEL_FUNC entry, ...)
#ifdef DEBUG
	push {r4, lr}
	mrs r4, ipsr
	cmp r4, #11
	beq __kernel_panic
	svc #0
	pop {r4, pc}
#else
	push {lr}
	svc #0
	pop {pc}
#endif

	.thumb_func
__mem_copy:	// r0: start, r1: stop, r2: source  
	cmp r0, r2
	beq 2f
	subs r1, r1, r0
	beq 2f
1:	ldrb r3, [r2]
	add r2, r2, #1
	strb r3, [r0]
	add r0, r0, #1
	sub r1, r1, #1
	bne 1b
2:	bx lr

		.thumb_func
__mem_set:	// r0: start, r1: stop, r2: stuff byte 
	cmp r0, r1
	beq 1f
	strb r2, [r0]
	add r0, r0, #1
	b __mem_set
1:	bx lr

