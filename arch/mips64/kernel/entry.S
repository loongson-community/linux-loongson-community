/* $Id: entry.S,v 1.6 1999/11/23 17:12:49 ralf Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Low level exception handling
 *
 * Copyright (C) 1994 - 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics
 */
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

/*
 * Heia ...  The %lo, %hi and %HI stuff is too strong for the ELF assembler
 * and the ABI to cope with ...
 */
		.text
		.set	noreorder
		.align 4
FEXPORT(ret_from_fork)
		jal	schedule_tail
		 move	a0, v0					# prev
		j	ret_from_sys_call
		 nop
FEXPORT(handle_bottom_half)
		jal	do_bottom_half
		 nop
		b	9f
		 nop

reschedule:	jal	schedule 
		 nop

FEXPORT(ret_from_sys_call)
FEXPORT(ret_from_irq)
		ld	t0, bh_mask
		ld	t1, bh_active		# unused delay slot
		and	t0, t1
		bnez	t0, handle_bottom_half
9:		 ld	t0, PT_STATUS(sp)	# returning to kernel mode?

		andi	t1, t0, 0x10
		beqz	t1, return		# -> yes
		 ld	t1, TASK_NEED_RESCHED($28)
		bnez	t1, reschedule
		lw	v0, TASK_SIGPENDING($28)
		 move	a0, zero
		beqz	v0, return
		 nop
		jal	do_signal
		 move	a1, sp

FEXPORT(return)	.set	noat
		RESTORE_ALL
		eret
		.set	at

/*
 * Common spurious interrupt handler.
 */
		.text
		.align  5
LEAF(spurious_interrupt)
		/*
		 * Someone tried to fool us by sending an interrupt but we
		 * couldn't find a cause for it.
		 */
		lui     t1,%hi(spurious_count)
		lw      t0,%lo(spurious_count)(t1)
		addiu   t0,1
		j	ret_from_irq
		 sw      t0,%lo(spurious_count)(t1)
		END(spurious_interrupt)
