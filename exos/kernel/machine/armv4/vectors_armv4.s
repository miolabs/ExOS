	.section .vectors, "ax"
	.code 32
	.global _vectors

_vectors:
	ldr pc, [pc, #reset_handler_address - . - 8]
	ldr pc, [pc, #undef_handler_address - . - 8]
	ldr pc, [pc, #swi_handler_address - . - 8]
	ldr pc, [pc, #pabort_handler_address - . - 8]
	ldr pc, [pc, #dabort_handler_address - . - 8]
#ifdef ARMV4_VECTORS_CHECKSUM
	.word ARMV4_VECTORS_CHECKSUM
#else
	nop
#endif
	ldr pc, [pc, #irq_handler_address - . - 8]
	ldr pc, [pc, #fiq_handler_address - . - 8]

reset_handler_address:
	.word Reset_Handler
undef_handler_address:
	.word Undef_Handler
swi_handler_address:
	.word SWI_Handler
pabort_handler_address:
	.word PAbort_Handler
dabort_handler_address:
	.word DAbort_Handler
irq_handler_address:
	.word IRQ_Handler
fiq_handler_address:
	.word FIQ_Handler



	.section .init, "ax"
	.code 32

__div0:
	b .	 /* Endless loop */

Undef_Handler:
	b .  /* Endless loop */

SWI_Handler:
	b .  /* Endless loop */

PAbort_Handler:
	b .  /* Endless loop */

DAbort_Handler:
	b .  /* Endless loop */
	subs pc, r14, #8	// return from dabort exception

IRQ_Handler:
	b .  /* Endless loop */

FIQ_Handler:
	b .  /* Endless loop */

	.weak __div0, Undef_Handler, SWI_Handler, PAbort_Handler, DAbort_Handler, IRQ_Handler, FIQ_Handler
