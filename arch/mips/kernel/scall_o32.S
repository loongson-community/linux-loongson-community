/*
 * Handle ABI O32 style syscalls.
 *
 * Copyright (C) 1997, 1998 by Ralf Baechle
 *
 * $Id: scall_o32.S,v 1.1 1998/03/12 19:06:20 ralf Exp $
 */
#include <asm/asm.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/unistd.h>

/* This duplicates the definition from <linux/sched.h> */
#define PF_TRACESYS	0x00000020	/* tracing system calls */

/* This duplicates the definition from <asm/signal.h> */
#define SIGILL		4		/* Illegal instruction (ANSI).  */

/* Highest syscall used of any syscall flavour */
#define MAX_SYSCALL_NO	__NR_Linux + __NR_Linux_syscalls

	.align  5
NESTED(handle_sys, PT_SIZE, sp)
	.set	noat
	SAVE_ALL
	STI
	.set	at

	/*
	 * By convention "li v0,<syscallno>" is always preceeding
	 * the syscall instruction.  So if we're in a delay slot
	 * userland is screwed up.
	 */
	lw	t0, PT_CAUSE(sp)	# delay slot?
	lw	t1, PT_EPC(sp)		# skip syscall on return
	bltz	t0, sigill_and_out

	sltiu	t0, v0, MAX_SYSCALL_NO + 1 # check syscall number
	addiu	t1, 4			# skip to next instruction
	beqz	t0, illegal_syscall
	sw	t1, PT_EPC(sp)

	/* XXX Put both in one cacheline, should save a bit. */
	sll	t0, v0, 2
	lw	s0, sys_call_table(t0)	# syscall routine
	lbu	s1, sys_narg_table(v0)	# number of arguments
	beqz	s0, illegal_syscall;

	subu	t0, s1, 5		# 5 or more arguments?
	bgezal	t0, stackargs

	lw	s3, TASK_FLAGS($28)	# syscall tracing enabled?
	andi	s3, PF_TRACESYS
	bnez	s3, trace_a_syscall

	jalr	s0			# Do The Real Thing (TM)

	li	t0, -EMAXERRNO - 1	# error?
	sltu	t0, t0, v0
	sw	t0, PT_R7(sp)		# set error flag
	beqz	t0, 1f

	negu	v0			# error
	sw	v0, PT_R0(sp)		# set flag for syscall restarting
1:	sw	v0, PT_R2(sp)		# result
	j	ret_from_sys_call

/* ------------------------------------------------------------------------ */

trace_a_syscall:
	jal	syscall_trace

	jalr	s0			# Do The Real Thing (TM)

	li	t0, -EMAXERRNO - 1	# error?
	sltu	t0, t0, v0
	sw	t0, PT_R7(sp)		# set error flag
	beqz	t0, 1f

	negu	v0			# error
	sw	v0, PT_R0(sp)		# set flag for syscall restarting
1:	sw	v0, PT_R2(sp)		# result

	jal	syscall_trace
	j	ret_from_sys_call

/* ------------------------------------------------------------------------ */

	/*
	 * More than four arguments.  Try to deal with it by copying the
	 * stack arguments from the user stack to the kernel stack.
	 * This Sucks (TM).
	 */
stackargs:
	move	s3, ra			# save return address

	lw	t0, PT_R29(sp)		# get old user stack pointer
	subu	s2, s1, 4
	sll	t1, s2, 2		# stack valid?

	addu	t1, t0			# end address
	or	t2, t0, t1
	bltz	t0, bad_stack		# -> sp is bad

	lw	t0, PT_R29(sp)		# get old user stack pointer
	la	t1, 3f			# copy 1 to 2 arguments
	sll	s2, s2, 3
	subu	t1, s2
	jr	t1

	/* Ok, copy the args from the luser stack to the kernel stack */
1:	lw	t1, 20(t0)		# argument #6 from usp
	sw	t1, 20(sp)
2:	lw	t1, 16(t0)		# argument #5 from usp
	sw	t1, 16(sp)

3:	jr	s3			# go back

	.section __ex_table,"a"
	PTR	1b,bad_stack
	PTR	2b,bad_stack
	.previous

	/*
	 * The stackpointer for a call with more than 4 arguments is bad.
	 */
bad_stack:
	negu	v0				# error
	sw	v0, PT_R0(sp)
	sw	v0, PT_R2(sp)
	li	t0, 1				# set error flag
	sw	t0, PT_R7(sp)
	j	ret_from_sys_call

	/*
	 * The system call does not exist in this kernel
	 */
illegal_syscall:
	li	v0, ENOSYS			# error
	sw	v0, PT_R2(sp)
	li	t0, 1				# set error flag
	sw	t0, PT_R7(sp)
	j	ret_from_sys_call

sigill_and_out:
	li	t0, -1				# not a sys call
	REG_S	t0, PT_OR2(sp)
	li	a0, SIGILL
	move	a2, $28
	jal	force_sig
	j	ret_from_sys_call
	END(handle_sys)
