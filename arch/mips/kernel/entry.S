/*
 * Low level exception handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 */

/*
 * entry.S contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch. The ISA dependent TLB
 * code is in arch/mips/<ISA-level>/<cputype>.S
 */
#include <linux/sys.h>

#include <asm/asm.h>
#include <asm/errno.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/pgtable.h>
#include <asm/stackframe.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/fpregdef.h>
#include <asm/unistd.h>

/*
 * These are offsets into the task-struct.
 */
state		=  0
counter		=  4
priority	=  8
signal		= 12
blocked		= 16
flags		= 20
errno		= 24
exec_domain	= 60

#ifdef __SMP__
#error "Fix this for SMP"
#else
#define current current_set
#endif

/*
 * Heia ...  The %lo, %hi and %HI stuff is too strong for the ELF assembler
 * and the ABI to cope with ...
 */
		.text
		.set	noreorder
		.align 4
handle_bottom_half:
		lui	s0,%hi(intr_count)
		lw	s1,%lo(intr_count)(s0)
		mfc0	s3,CP0_STATUS		# Enable IRQs
		addiu	s2,s1,1
		sw	s2,%lo(intr_count)(s0)
		ori	t0,s3,0x1f
		xori	t0,0x1e
		jal	do_bottom_half
		mtc0	t0,CP0_STATUS		# delay slot
		mtc0	s3,CP0_STATUS		# Restore old IRQ state
		b	9f
		sw	s1,%lo(intr_count)(s0)	# delay slot

reschedule:	jal	schedule 
		nop				# delay slot
EXPORT(ret_from_sys_call)
		lw	t0,intr_count		# bottom half
		bnez	t0,return

9:		lw	t0,bh_mask		# delay slot
		lw	t1,bh_active		# unused delay slot
		and	t0,t1
		bnez	t0,handle_bottom_half

		lw	t0,FR_STATUS(sp)	# returning to kernel mode?
		andi	t1,t0,0x10
		beqz	t1,return		# -> yes

		mfc0	t0,CP0_STATUS		# delay slot
		lw	t1,need_resched
		ori	t0,0x1f			# enable irqs
		xori	t0,0x1e
		bnez	t1,reschedule
		mtc0	t0,CP0_STATUS		# delay slot

		lw	s0,current
		lw	t0,task
		lw	a0,blocked(s0)
		beq	s0,t0,return		# task[0] cannot have signals
						# save blocked in a0 for
						# signal handling
		lw	t0,signal(s0)		# delay slot
		nor	t1,zero,a0
		and	t1,t0,t1
		beqz	t1,return
		nop

		jal	do_signal
		move	a1,sp			# delay slot

		.set	noat
EXPORT(return)
		RESTORE_ALL
		ERET
		.set	at

/*
 * Beware: timer_interrupt, interrupt, fast_interrupt and bad_interrupt
 * have unusual calling conventions to speedup the mess.
 *
 * a0 - interrupt number
 * s2 - destroyed
 * return values:
 * v0 - return routine
 *
 * The timer interrupt is handled specially to insure that the jiffies
 * variable is updated at all times.  Specifically, the timer interrupt is
 * just like the complete handlers except that it is invoked with interrupts
 * disabled and should never re-enable them.  If other interrupts were
 * allowed to be processed while the timer interrupt is active, then the
 * other interrupts would have to avoid using the jiffies variable for delay
 * and interval timing operations to avoid hanging the system.
 */
		.text
		.set	at
		.align	5
NESTED(timer_interrupt, FR_SIZE, sp)
		move	s2,ra
		jal	do_IRQ
		move	a1,sp				# delay slot
		.set	reorder
		la	v0,ret_from_sys_call
		jr	s2
		.set	noreorder
		END(timer_interrupt)

		.align	5
NESTED(interrupt, FR_SIZE, sp)
		move	s2,ra
		mfc0	t0,CP0_STATUS			# enable IRQs
		ori	t0,0x1f
		xori	t0,0x1e
		mtc0	t0,CP0_STATUS
		jal	do_IRQ
		move	a1,sp				# delay slot
		mfc0	t0,CP0_STATUS			# disable IRQs
		ori	t0,1
		xori	t0,1
		mtc0	t0,CP0_STATUS
		.set	reorder
		la	v0,ret_from_sys_call
		jr	s2
		.set	noreorder
		END(interrupt)

		.align	5
NESTED(fast_interrupt, FR_SIZE, sp)
		.set	reorder
		move	s2,ra
		jal	do_fast_IRQ
		la	v0,return
		jr	s2
		.set	noreorder
		END(fast_interrupt)

		/*
		 * Don't return & unblock the pic
		 */
LEAF(bad_interrupt)
		.set	reorder
		lw	t0,%lo(intr_count)(s3)
		subu	t0,1
		.set	noreorder
		j	return
		sw	t0,%lo(intr_count)(s3)		# delay slot
		END(bad_interrupt)

		.text
		.align  5
LEAF(spurious_interrupt)
		/*
		 * Someone tried to fool us by sending an interrupt but we
		 * couldn't find a cause for it.
		 */
		lui     t1,%hi(spurious_count)
		lw      t0,%lo(spurious_count)(t1)
		la      v0,return
		addiu   t0,1
		jr      ra
		sw      t0,%lo(spurious_count)(t1)
		END(spurious_interrupt)

/*
 * Build a default exception handler for the exceptions that don't need
 * special handlers.  If you didn't know yet - I *like* playing games with
 * the C preprocessor ...
 */
#define __BUILD_clear_none(exception)                                   \
		REG_S	sp,FR_ORIG_REG2(sp);    /* sp < 0 */
#define __BUILD_clear_sys(exception)                                    \
		REG_S	v0,FR_ORIG_REG2(sp);                            \
		REG_S	a3,FR_ORIG_REG7(sp);
#define __BUILD_clear_fpe(exception)                                    \
		REG_S	sp,FR_ORIG_REG2(sp);    /* sp < 0 */            \
		cfc1	a1,fcr31;                                       \
		li	a2,~(0x3f<<12);                                 \
		and	a2,a1;                                          \
		ctc1	a2,fcr31;
#define __BUILD_clear_watch(exception)                                  \
		REG_S	sp,FR_ORIG_REG2(sp);    /* sp < 0 */            \
		mtc0	zero,CP0_WATCHLO;                               \
		mtc0	zero,CP0_WATCHHI
#define __BUILD_clear_ade(exception)                                    \
		REG_S	sp,FR_ORIG_REG2(sp);    /* sp < 0 */            \
		MFC0	t0,CP0_BADVADDR;                                \
		REG_S	t0,FR_BADVADDR(sp);
#define __BUILD_silent(exception)
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
#define fmt "Got %s at %08x.\n"
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
#define fmt "Got %s at %016Lx.\n"
#endif
#define __BUILD_verbose(exception)                                      \
		la	a1,8f;                                          \
		TEXT	(#exception);                                   \
		REG_L	a2,FR_EPC(sp);                                  \
		PRINT(fmt)
#define __BUILD_count(exception)                                        \
		.set	reorder;                                        \
		lw	t0,exception_count_##exception;                 \
		addiu	t0,1;                                           \
		sw	t0,exception_count_##exception;                 \
		.set	noreorder;                                      \
		.data;                                                  \
EXPORT(exception_count_##exception);                                    \
		.word	0;                                              \
		.text;
#define BUILD_HANDLER(exception,handler,clear,verbose)                  \
		.text;                                                  \
		.align	5;                                              \
		NESTED(handle_##exception, FR_SIZE, sp);                \
		.set	noat;                                           \
		SAVE_ALL;                                               \
		__BUILD_clear_##clear(exception);                       \
		STI;                                                    \
		.set	at;                                             \
		__BUILD_##verbose(exception);                           \
		REG_S	sp,FR_ORIG_REG2(sp);     /* not a sys call */   \
		jal	do_##handler;                                   \
		move	a0,sp;                   /* delay slot */       \
		j	ret_from_sys_call;                              \
		nop;                             /* delay slot */       \
		END(handle_##exception)

		BUILD_HANDLER(adel,ade,ade,silent)		/* #4  */
		BUILD_HANDLER(ades,ade,ade,silent)		/* #5  */
		BUILD_HANDLER(ibe,ibe,none,verbose)		/* #6  */
		BUILD_HANDLER(dbe,dbe,none,verbose)		/* #7  */
		BUILD_HANDLER(sys,sys,none,silent)		/* #8  */
		BUILD_HANDLER(bp,bp,none,silent)		/* #9  */
		BUILD_HANDLER(ri,ri,none,silent)		/* #10 */
		BUILD_HANDLER(cpu,cpu,none,silent)		/* #11 */
		BUILD_HANDLER(ov,ov,none,silent)		/* #12 */
		BUILD_HANDLER(tr,tr,none,silent)		/* #13 */
		BUILD_HANDLER(vcei,vcei,none,verbose)		/* #14 */
		BUILD_HANDLER(fpe,fpe,fpe,silent)		/* #15 */
		BUILD_HANDLER(watch,watch,watch,verbose)	/* #23 */
		BUILD_HANDLER(vced,vced,none,verbose)		/* #31 */
		BUILD_HANDLER(reserved,reserved,none,verbose)	/* others */

/*
 * Exception handler table with 32 entries.
 * This might be extended to handle software exceptions
 */
		.bss
		.align	PTRLOG
EXPORT(exception_handlers)
		.fill	32,PTRSIZE,0

/*
 * Interrupt handler table with 16 entries.
 */
EXPORT(IRQ_vectors)
		.fill	16,PTRSIZE,0

/*
 * Table of syscalls
 */
		.data
		.align	PTRLOG
EXPORT(sys_call_table)
		/*
		 * Reserved space for all the SVR4, SVR, BSD43 and POSIX
		 * flavoured syscalls.
		 */
		.space	(__NR_Linux)*PTRSIZE

		/*
		 * Linux flavoured syscalls.
		 */
#define SYS(call, narg) PTR call
#include "syscalls.h"

/*
 * Number of arguments of each syscall
 * FIXME: This table contains huge empty areas wasting memory.
 */
EXPORT(sys_narg_table)
		/*
		 * Reserved space for all the SVR4, SVR, BSD43 and POSIX
		 * flavoured syscalls.
		 */
		.space	(__NR_Linux)

		/*
		 * Linux flavoured syscalls.
		 */
#undef SYS
#define SYS(call, narg) .byte narg
#include "syscalls.h"
