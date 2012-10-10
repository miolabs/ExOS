/*****************************************************************************
  Exception handlers and startup code for a generic ARM target.

  Copyright (c) 2005 Rowley Associates Limited.

  This file may be distributed under the terms of the License Agreement
  provided with this software.

  THIS FILE IS PROVIDED AS IS WITH NO WARRANTY OF ANY KIND, INCLUDING THE
  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *****************************************************************************/

/*****************************************************************************
 *                           Preprocessor Definitions
 *                           ------------------------
 *
 * STARTUP_FROM_RESET
 *
 *   If defined, the program will startup from power-on/reset. If not defined
 *   the program will just loop endlessly from power-on/reset.
 *
 *   This definition is not defined by default on this target because the
 *   debugger is unable to reset this target and maintain control of it over the
 *   JTAG interface. The advantage of doing this is that it allows the debugger
 *   to reset the CPU and run programs from a known reset CPU state on each run.
 *   It also acts as a safety net if you accidently download a program in FLASH
 *   that crashes and prevents the debugger from taking control over JTAG
 *   rendering the target unusable over JTAG. The obvious disadvantage of doing
 *   this is that your application will not startup without the debugger.
 *
 *   We advise that on this target you keep STARTUP_FROM_RESET undefined whilst
 *   you are developing and only define STARTUP_FROM_RESET when development is
 *   complete.
 *
 *****************************************************************************/

  .section .vectors, "ax"
  .code 32
  .align 0
  .global _vectors
  .global reset_handler
  .global __wait

/*****************************************************************************
 *                                                                           *
 * Exception Vectors                                                         *
 *                                                                           *
 *****************************************************************************/
_vectors:
#ifdef STARTUP_FROM_RESET
  ldr pc, [pc, #reset_handler_address - . - 8]  /* reset */
#else
  b .                                           /* reset - infinite loop */
#endif
  ldr pc, [pc, #undef_handler_address - . - 8]  /* undefined instruction */
  ldr pc, [pc, #swi_handler_address - . - 8]    /* swi handler */
  ldr pc, [pc, #pabort_handler_address - . - 8] /* abort prefetch */
  ldr pc, [pc, #dabort_handler_address - . - 8] /* abort data */
  nop
  ldr pc, [pc, #irq_handler_address - . - 8]    /* irq */
  ldr pc, [pc, #fiq_handler_address - . - 8]    /* fiq */

reset_handler_address:
  .word reset_handler
undef_handler_address:
  .word undef_handler
swi_handler_address:
  .word swi_handler
pabort_handler_address:
  .word pabort_handler
dabort_handler_address:
  .word dabort_handler
irq_handler_address:
  .word irq_handler
fiq_handler_address:
  .word fiq_handler

  .section .init, "ax"
  .code 32
  .align 0

/******************************************************************************
 *                                                                            *
 * Default exception handlers                                                 *
 *                                                                            *
 ******************************************************************************/

#define INTC_BASE 0x01C48000
#define EABASE_OFFSET 0x024
#define IRQ0_OFFSET 0x008
#define IRQ1_OFFSET 0x00C

reset_handler:
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
   	orr r1, #1	// entry size = 8
	str r1, [r2, #EABASE_OFFSET]
	ldr r1, =0xffffffff
	str r1, [r2, #IRQ0_OFFSET]
	str r1, [r2, #IRQ1_OFFSET]

	b _start


.macro DEFAULT_ISR_HANDLER name=
  .weak \name
\name:
1:	b 1b /* endless loop */
.endm

IRQTable:	.word 0	// should not be used
			.word 0
			.word INT00_Handler
			.word 0
			.word INT01_Handler
			.word 1
			.word INT02_Handler
			.word 2
			.word INT03_Handler
			.word 3
			.word INT04_Handler
			.word 4
			.word INT05_Handler
			.word 5
			.word INT06_Handler
			.word 6
			.word INT07_Handler
			.word 7
			.word INT08_Handler
			.word 8
			.word INT09_Handler
			.word 9
			.word INT10_Handler
			.word 10
			.word INT11_Handler
			.word 11
			.word INT12_Handler
			.word 12
			.word INT13_Handler
			.word 13
			.word INT14_Handler
			.word 14
			.word INT15_Handler
			.word 15
			.word INT16_Handler
			.word 16
			.word INT17_Handler
			.word 17
			.word INT18_Handler
			.word 18
			.word INT19_Handler
			.word 19
			.word INT20_Handler
			.word 20
			.word INT21_Handler
			.word 21
			.word INT22_Handler
			.word 22
			.word INT23_Handler
			.word 23
			.word INT24_Handler
			.word 24
			.word INT25_Handler
			.word 25
			.word INT26_Handler
			.word 26
			.word INT27_Handler
			.word 27
			.word INT28_Handler
			.word 28
			.word INT29_Handler
			.word 29
			.word INT30_Handler
			.word 30
			.word INT31_Handler
			.word 31
			.word INT32_Handler
			.word 32
			.word INT33_Handler
			.word 33
			.word INT34_Handler
			.word 34
			.word INT35_Handler
			.word 35
			.word INT36_Handler
			.word 36
			.word INT37_Handler
			.word 37
			.word INT38_Handler
			.word 38
			.word INT39_Handler
			.word 39
			.word INT40_Handler
			.word 40
			.word INT41_Handler
			.word 41
			.word INT42_Handler
			.word 42
			.word INT43_Handler
			.word 43
			.word INT44_Handler
			.word 44
			.word INT45_Handler
			.word 45
			.word INT46_Handler
			.word 46
			.word INT47_Handler
			.word 47
			.word INT48_Handler
			.word 48
			.word INT49_Handler
			.word 49
			.word INT50_Handler
			.word 50
			.word INT51_Handler
			.word 51
			.word INT52_Handler
			.word 52
			.word INT53_Handler
			.word 53
			.word INT54_Handler
			.word 54
			.word INT55_Handler
			.word 55
			.word INT56_Handler
			.word 56
			.word INT57_Handler
			.word 57
			.word INT58_Handler
			.word 58
			.word INT59_Handler
			.word 59
			.word INT60_Handler
			.word 60
			.word INT61_Handler
			.word 61
			.word INT62_Handler
			.word 62
			.word INT63_Handler
			.word 63

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


__wait:	subs r0, #1
		bne __wait
		mov pc, lr

/******************************************************************************
 *                                                                            *
 * Default exception handlers                                                 *
 * These are declared weak symbols so they can be redefined in user code.     * 
 *                                                                            *
 ******************************************************************************/

undef_handler:
  b .  /* Endless loop */

swi_handler:
  b .  /* Endless loop */

pabort_handler:
  b .  /* Endless loop */

dabort_handler:
  b .  /* Endless loop */

irq_handler:
  b .  /* Endless loop */

fiq_handler:
  b .  /* Endless loop */

  .weak undef_handler, swi_handler, pabort_handler, dabort_handler, irq_handler, fiq_handler

