/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999, 2000 by Ralf Baechle and others.
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/mman.h>
#include <linux/personality.h>
#include <linux/sys.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/bootinfo.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mipsregs.h>
#include <asm/processor.h>
#include <asm/stackframe.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/elf.h>
#include <asm/cpu.h>
#include <asm/fpu.h>

/*
 * We use this if we don't have any better idle routine..
 * (This to kill: kernel/platform.c.
 */
void default_idle (void)
{
}

/*
 * The idle thread. There's no useful work to be done, so just try to conserve
 * power and have a low exit latency (ie sit in a loop waiting for somebody to
 * say that they'd like to reschedule)
 */
ATTRIB_NORET void cpu_idle(void)
{
	/* endless idle loop with no priority at all */
	while (1) {
		while (!need_resched())
			if (cpu_wait)
				(*cpu_wait)();
		schedule();
	}
}

asmlinkage void ret_from_fork(void);

void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp)
{
	unsigned long status;

	/* New thread looses kernel privileges. */
	status = regs->cp0_status & ~(ST0_CU0|ST0_FR|ST0_KSU);
	status |= KSU_USER;
	status |= (current->thread.mflags & MF_32BIT) ? 0 : ST0_FR;
	regs->cp0_status = status;
	current->used_math = 0;
	loose_fpu();
	regs->cp0_epc = pc;
	regs->regs[29] = sp;
	current_thread_info()->addr_limit = USER_DS;
}

void exit_thread(void)
{
}

void flush_thread(void)
{
}

int copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
		unsigned long unused, struct task_struct *p,
		struct pt_regs *regs)
{
	struct thread_info *ti = p->thread_info;
	struct pt_regs *childregs;
	long childksp;

	childksp = (unsigned long)ti + KERNEL_STACK_SIZE - 32;

	if (is_fpu_owner()) {
		save_fp(p);
	}

	/* set up new TSS. */
	childregs = (struct pt_regs *) childksp - 1;
	*childregs = *regs;
	childregs->regs[7] = 0;	/* Clear error flag */
	childregs->regs[2] = 0;	/* Child gets zero as return value */
	regs->regs[2] = p->pid;

	if (childregs->cp0_status & ST0_CU0) {
		childregs->regs[28] = (unsigned long) ti;
		childregs->regs[29] = childksp;
		ti->addr_limit = KERNEL_DS;
	} else {
		childregs->regs[29] = usp;
		ti->addr_limit = USER_DS;
	}
	p->thread.reg29 = (unsigned long) childregs;
	p->thread.reg31 = (unsigned long) ret_from_fork;

	/*
	 * New tasks loose permission to use the fpu. This accelerates context
	 * switching for most programs since they don't use the fpu.
	 */
	p->thread.cp0_status = read_32bit_cp0_register(CP0_STATUS) &
                            ~(ST0_CU3|ST0_CU2|ST0_CU1|ST0_KSU);
	childregs->cp0_status &= ~(ST0_CU3|ST0_CU2|ST0_CU1);

	return 0;
}

/* Fill in the fpu structure for a core dump.. */
int dump_fpu(struct pt_regs *regs, elf_fpregset_t *r)
{
	memcpy(r, &current->thread.fpu, sizeof(current->thread.fpu));
	return 1;
}

/*
 * Create a kernel thread
 */
int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	int retval;

	__asm__ __volatile__(
		"move\t$6, $sp\n\t"
		"move\t$4, %5\n\t"
		"li\t$2, %1\n\t"
		"syscall\n\t"
		"beq\t$6, $sp, 1f\n\t"
		"move\t$4, %3\n\t"
		"jalr\t%4\n\t"
		"move\t$4, $2\n\t"
		"li\t$2, %2\n\t"
		"syscall\n"
		"1:\tmove\t%0, $2"
		:"=r" (retval)
		:"i" (__NR_clone), "i" (__NR_exit), "r" (arg), "r" (fn),
		 "r" (flags | CLONE_VM | CLONE_UNTRACED)

		 /* The called subroutine might have destroyed any of the
		  * at, result, argument or temporary registers ...  */
		: "$2", "$3", "$4", "$5", "$6", "$7", "$8",
		  "$9","$10","$11","$12","$13","$14","$15","$24","$25");

	return retval;
}

/*
 * Return saved PC of a blocked thread.
 */
unsigned long thread_saved_pc(struct thread_struct *t)
{
	extern void ret_from_fork(void);

	/* New born processes are a special case */
	if (t->reg31 == (unsigned long) ret_from_fork)
		return t->reg31;

	return ((unsigned long*)t->reg29)[11];
}

/*
 * These bracket the sleeping functions..
 */
extern void scheduling_functions_start_here(void);
extern void scheduling_functions_end_here(void);
#define first_sched	((unsigned long) scheduling_functions_start_here)
#define last_sched	((unsigned long) scheduling_functions_end_here)

/* get_wchan - a maintenance nightmare ...  */
unsigned long get_wchan(struct task_struct *p)
{
	unsigned long frame, pc;

	if (!p || p == current || p->state == TASK_RUNNING)
		return 0;

	pc = thread_saved_pc(&p->thread);
	if (pc < first_sched || pc >= last_sched)
		goto out;

	if (pc >= (unsigned long) sleep_on_timeout)
		goto schedule_timeout_caller;
	if (pc >= (unsigned long) sleep_on)
		goto schedule_caller;
	if (pc >= (unsigned long) interruptible_sleep_on_timeout)
		goto schedule_timeout_caller;
	if (pc >= (unsigned long)interruptible_sleep_on)
		goto schedule_caller;
	goto schedule_timeout_caller;

schedule_caller:
	frame = ((unsigned long *)p->thread.reg30)[10];
	pc    = ((unsigned long *)frame)[7];
	goto out;

schedule_timeout_caller:
	/* Must be schedule_timeout ...  */
	pc    = ((unsigned long *)p->thread.reg30)[11];
	frame = ((unsigned long *)p->thread.reg30)[10];

	/* The schedule_timeout frame ...  */
	pc    = ((unsigned long *)frame)[9];
	frame = ((unsigned long *)frame)[8];

	if (pc >= first_sched && pc < last_sched) {
		/* schedule_timeout called by interruptible_sleep_on_timeout */
		pc    = ((unsigned long *)frame)[7];
		frame = ((unsigned long *)frame)[6];
	}

out:
	if (current->thread.mflags & MF_32BIT)	/* Kludge for 32-bit ps  */
		pc &= 0xffffffff;

	return pc;
}
