/*
 * int-handler.S: Interrupt exception dispatch code for Philips Nino
 *
 * Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 */

#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

	/*
	 * Here is the table of interrupts for the Philips Nino
	 * which uses the Philips PR31700/Toshiba TMPR3912 core.
	 *
	 *	MIPS IRQ	PR31700 IRQ	Source
	 *      --------        -----------	------
	 *             0	          1	Sound, LCD, telecom
	 *             1	          2	UARTA & UARTB
	 *             2	          3	MFIO (positive edge)
	 *             3	          4	MFIO (negative edge)
	 *             4	          5	Timers, power management
	 *             5	          6	High priority interrupts
	 *
	 * We handle the IRQ according to the priorities below:
	 *
	 * Highest ----     High priority interrupts
	 *                  UARTA & UARTB
	 *                  Timers, power management
	 *                  Sound, LCD, telecom
	 *                  Multi-function IO (positive edge)
	 * Lowest  ----     Multi-function IO (negative edge)
	 *
	 * then we just return, if multiple IRQs are pending then
	 * we will just take another exception, big deal.
	 */

	.text
	.set	noreorder
	.set	noat
	.align	5
	NESTED(ninoIRQ, PT_SIZE, sp)
	SAVE_ALL
	CLI
	.set	at
	mfc0	s0, CP0_CAUSE		# get irq mask

	/* Check for IRQ4 */
	andi	a0, s0, C_IRQ4
	beq	a0, zero, 1f
	andi	a0, s0, C_IRQ2		# delay slot, check for IRQ2

	/* High priority interrupt */
	move    a0, sp
	jal	irq4_dispatch
	nop				# delay slot

	j	ret_from_irq
	nop				# delay slot

1:
	/* Check for IRQ2 */
	beq	a0, zero, 1f
	nop				# delay slot

	/* UART interrupt of some sort */
	move	a0, sp
	jal	irq2_dispatch
	nop				# delay slot

	j	ret_from_irq
	nop				# delay slot

1:
	/* We should never get here */
	move	a0, sp
	jal	irq_bad
	nop

	j	ret_from_irq
	nop
	END(ninoIRQ)
