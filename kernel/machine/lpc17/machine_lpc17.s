	.section .text


	.global __mem_copy
	.global __mem_set

	.syntax unified
	.code 16


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

