/*
 *  linux/arch/mips/kernel/process.c
 *
 *  Copyright (C) 1995 Ralf Baechle
 *  written by Ralf Baechle
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/bootinfo.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mipsregs.h>
#include <asm/processor.h>
#include <asm/stackframe.h>
#include <asm/io.h>

asmlinkage void ret_from_sys_call(void);

asmlinkage int sys_pipe(unsigned long * fildes)
{
	int fd[2];
	int error;

	error = verify_area(VERIFY_WRITE,fildes,8);
	if (error)
		return error;
	error = do_pipe(fd);
	if (error)
		return error;
	put_fs_long(fd[0],0+fildes);
	put_fs_long(fd[1],1+fildes);
	return 0;
}

asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		/*
		 * R4[26]00 have wait, R4[04]00 don't.
		 */
		if (wait_available && !need_resched)
			__asm__(".set\tmips3\n\t"
				"wait\n\t"
				".set\tmips0\n\t");
		schedule();
	}
}

/*
 * This routine reboots the machine by asking the keyboard
 * controller to pulse the reset-line low. We try that for a while,
 * and if it doesn't work, we do some other stupid things.
 * Should be ok for Deskstation Tynes. Reseting others needs to be
 * investigated...
 */
static inline void kb_wait(void)
{
	int i;

	for (i=0; i<0x10000; i++)
		if ((inb_p(0x64) & 0x02) == 0)
			break;
}

/*
 * Hard reset for Deskstation Tyne
 * No hint how this works on Pica boards.
 */
void hard_reset_now(void)
{
	int i, j;

	sti();
	for (;;) {
		for (i=0; i<100; i++) {
			kb_wait();
			for(j = 0; j < 100000 ; j++)
				/* nothing */;
			outb(0xfe,0x64);	 /* pulse reset low */
		}
	}
}

void show_regs(struct pt_regs * regs)
{
	/*
	 * Saved main processor registers
	 */
	printk("$0 : %08x %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       0, regs->reg1, regs->reg2, regs->reg3,
               regs->reg4, regs->reg5, regs->reg6, regs->reg7);
	printk("$8 : %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       regs->reg8, regs->reg9, regs->reg10, regs->reg11,
               regs->reg12, regs->reg13, regs->reg14, regs->reg15);
	printk("$16: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       regs->reg16, regs->reg17, regs->reg18, regs->reg19,
               regs->reg20, regs->reg21, regs->reg22, regs->reg23);
	printk("$24: %08lx %08lx                   %08lx %08lx %08lx %08lx\n",
	       regs->reg24, regs->reg25, regs->reg28, regs->reg29,
               regs->reg30, regs->reg31);

	/*
	 * Saved cp0 registers
	 */
	printk("epc  : %08lx\nStatus: %08lx\nCause : %08lx\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause);
}

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

void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
                          struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;

	/*
	 * set up new TSS
	 */
	childregs = ((struct pt_regs *) (p->kernel_stack_page + PAGE_SIZE)) - 1;
	*childregs = *regs;
	childregs->reg2 = 0;
	regs->reg2 = p->pid;
	childregs->reg29 = usp;
	p->tss.ksp = (p->kernel_stack_page + PAGE_SIZE - 8);
	p->tss.reg29 = (unsigned long) childregs;	/* new sp */
	p->tss.reg31 = (unsigned long) ret_from_sys_call;

	/*
	 * New tasks loose permission to use the fpu. This accelerates context
	 * switching for most programs since they don't use the fpu.
	 */
	p->tss.cp0_status = read_32bit_cp0_register(CP0_STATUS) &
                            ~(ST0_CU3|ST0_CU2|ST0_CU1|ST0_KSU|ST0_ERL|ST0_EXL);
	childregs->cp0_status &= ~(ST0_CU3|ST0_CU2|ST0_CU1);
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

asmlinkage int sys_fork(struct pt_regs regs)
{
	return do_fork(COPYVM | SIGCHLD, regs.reg29, &regs);
}

asmlinkage int sys_clone(struct pt_regs regs)
{
	unsigned long clone_flags;
	unsigned long newsp;

	newsp = regs.reg4;
	clone_flags = regs.reg5;
	if (!newsp)
		newsp = regs.reg29;
	if (newsp == regs.reg29)
		clone_flags |= COPYVM;
	return do_fork(clone_flags, newsp, &regs);
}

/*
 * sys_execve() executes a new program.
 */
asmlinkage int sys_execve(struct pt_regs regs)
{
	int error;
	char * filename;

	error = getname((char *) regs.reg4, &filename);
	if (error)
		return error;
	error = do_execve(filename, (char **) regs.reg5, (char **) regs.reg6, &regs);
	putname(filename);
	return error;
}
