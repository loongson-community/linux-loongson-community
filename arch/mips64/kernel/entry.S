/* $Id$
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
#include <linux/config.h>
#include <linux/sys.h>

#include <asm/asm.h>
#include <asm/current.h>
#include <asm/errno.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/stackframe.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/fpregdef.h>
#include <asm/unistd.h>

/*
 * Heia ...  The %lo, %hi and %HI stuff is too strong for the ELF assembler
 * and the ABI to cope with ...
 */
		.text
		.set	noreorder
		.set	mips3
		.align 4
EXPORT(handle_bottom_half)
		jal	do_bottom_half
		 nop
		b	9f
		 nop

reschedule:	jal	schedule 
		 nop

EXPORT(ret_from_sys_call)
EXPORT(ret_from_irq)
		lw	t0,bh_mask
		lw	t1,bh_active		# unused delay slot
		and	t0,t1
		bnez	t0,handle_bottom_half
9:		 lw	t0,PT_STATUS(sp)	# returning to kernel mode?

		andi	t1, t0, 0x10
		beqz	t1, return		# -> yes
		 lw	t1, TASK_NEED_RESCHED($28)
		bnez	t1, reschedule
		lw	v0, TASK_SIGPENDING($28)
		 move	a0, zero
		beqz	v0, return
		 nop
		jal	do_signal
		 move	a1, sp

EXPORT(return)	.set	noat
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

/*
 * Build a default exception handler for the exceptions that don't need
 * special handlers.  If you didn't know yet - I *like* playing games with
 * the C preprocessor ...
 */
		.macro	__build_clear_none
		.endm

		.macro	__build_clear_sti
		STI
		.endm

		.macro	__build_clear_cli
		CLI
		.endm

		.macro	__build_clear_fpe
		cfc1	a1, fcr31
		li	a2, ~(0x3f << 13)
		and	a2, a1
		ctc	a2, fcr31
		STI
		.endm

		.macro	__build_clear_ade
		mfc0	t0, CP0_BADVADDR
		sd	t0, PT_BVADDR(sp)
		KMODE
		.endm

#define __BUILD_silent(exception)

#define fmt "Got %s at %08lx.\n"

#define __BUILD_verbose(exception)                                      \
		la	a1,8f;                                          \
		TEXT	(#exception);                                   \
		ld	a2, PT_EPC(sp);                                 \
		PRINT(fmt)
#define __BUILD_count(exception)                                        \
		.set	reorder;                                        \
		lw	t0,exception_count_##exception;                 \
		addiu	t0, 1;                                          \
		sw	t0,exception_count_##exception;                 \
		.set	noreorder;                                      \
		.data;                                                  \
EXPORT(exception_count_##exception);                                    \
		.word	0;                                              \
		.previous;
#define BUILD_HANDLER(exception,handler,clear,verbose)                  \
		.align	5;                                              \
		NESTED(handle_##exception, PT_SIZE, sp);                \
		.set	noat;                                           \
		SAVE_ALL;                                               \
		__BUILD_clear_##clear exception;                        \
		.set	at;                                             \
		__BUILD_##verbose(exception);                           \
		jal	do_##handler;                                   \
		 move	a0,sp;                                          \
		j	ret_from_sys_call;                              \
		 nop;                                                   \
		END(handle_##exception)

		BUILD_HANDLER(adel,ade,ade,silent)		/* #4  */
		BUILD_HANDLER(ades,ade,ade,silent)		/* #5  */
		BUILD_HANDLER(ibe,ibe,cli,verbose)		/* #6  */
		BUILD_HANDLER(dbe,dbe,cli,silent)		/* #7  */
		BUILD_HANDLER(bp,bp,sti,silent)			/* #9  */
		BUILD_HANDLER(ri,ri,sti,silent)			/* #10 */
		BUILD_HANDLER(cpu,cpu,sti,silent)		/* #11 */
		BUILD_HANDLER(ov,ov,sti,silent)			/* #12 */
		BUILD_HANDLER(tr,tr,sti,silent)			/* #13 */
		BUILD_HANDLER(fpe,fpe,fpe,silent)		/* #15 */
		BUILD_HANDLER(watch,watch,sti,verbose)		/* #23 */
		BUILD_HANDLER(reserved,reserved,sti,verbose)	/* others */
