/*
 *  linux/arch/mips/kernel/process.c
 *
 *  Copyright (C) 1995 Ralf Baechle
 *
 *  Modified for R3000/DECStation support by Paul M. Antoine 1995, 1996
 *
 * This file handles the architecture-dependent parts of initialization,
 * though it does not yet currently fully support the DECStation,
 * or R3000 - PMA.
 */
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/mman.h>
#include <linux/sys.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/sgidefs.h>
#include <asm/system.h>
#include <asm/mipsregs.h>
#include <asm/processor.h>
#include <asm/stackframe.h>
#include <asm/io.h>

asmlinkage void ret_from_sys_call(void);

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
	/*
	 * Nothing to do
	 */
}

void flush_thread(void)
{
	/*
	 * Nothing to do
	 */
}

void release_thread(struct task_struct *dead_task)
{
	/*
	 * Nothing to do
	 */
}
  
void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
                 struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;
	long childksp;

	childksp = p->kernel_stack_page + KERNEL_STACK_SIZE - 8;
	/*
	 * set up new TSS
	 */
	childregs = ((struct pt_regs *) (p->kernel_stack_page + PAGE_SIZE)) - 1;
	*childregs = *regs;
	childregs->regs[2] = (__register_t) 0;	/* Child gets zero as return value */
	childregs->regs[7] = (__register_t) 0;	/* Clear error flag */
	regs->regs[2] = (__register_t) p->pid;
	if (childregs->cp0_status & ST0_CU0)
		childregs->regs[29] = (__register_t) childksp;
	else
		childregs->regs[29] = (__register_t) usp;
	p->tss.ksp = childksp;
	p->tss.reg29 = (__register_t)(long) childregs;	/* new sp */
	p->tss.reg31 = (__register_t) ret_from_sys_call;

	/*
	 * Copy thread specific flags.
	 */
	p->tss.mflags = p->tss.mflags;

	/*
	 * New tasks loose permission to use the fpu. This accelerates context
	 * switching for most programs since they don't use the fpu.
	 */
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
	p->tss.cp0_status = read_32bit_cp0_register(CP0_STATUS) &
	                    ~(ST0_CU3|ST0_CU2|ST0_CU1);
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
	p->tss.cp0_status = read_32bit_cp0_register(CP0_STATUS) &
	                    ~(ST0_CU3|ST0_CU2|ST0_CU1|ST0_KSU|ST0_ERL|ST0_EXL);
#endif
	childregs->cp0_status &= ~(ST0_CU3|ST0_CU2|ST0_CU1);
}

/*
 * Do necessary setup to start up a newly executed thread.
 */
extern void (*switch_to_user_mode)(struct pt_regs *regs);

void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp)
{
	set_fs(USER_DS);
	regs->cp0_epc = (__register_t) pc;
	/*
	 * New thread loses kernel privileges.
	 */
	switch_to_user_mode(regs);
	regs->regs[29] = (__register_t) sp;
	regs->regs[31] = 0;
}

/*
 * fill in the fpu structure for a core dump..
 *
 * Actually this is "int dump_fpu (struct pt_regs * regs, struct user *fpu)"
 */
int dump_fpu (int shutup_the_gcc_warning_about_elf_fpregset_t)
{
	int fpvalid = 0;
	/*
	 * To do...
	 */

	return fpvalid;
}

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
	/*
	 * To do...
	 */
}
