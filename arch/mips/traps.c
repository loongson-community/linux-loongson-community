/*
 *  linux/kernel/traps.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * 'traps.c' handles hardware traps and faults after we have saved some
 * state in 'asm.s'. Currently mostly a debugging-aid, will be extended
 * to mainly kill the offending process (probably by giving it a signal,
 * but possibly by killing it outright if necessary).
 */
#include <linux/head.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ptrace.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/mipsregs.h>

static inline void console_verbose(void)
{
	extern int console_loglevel;
	console_loglevel = 15;
}

asmlinkage extern void handle_int(void);
asmlinkage extern void handle_mod(void);
asmlinkage extern void handle_tlbl(void);
asmlinkage extern void handle_tlbs(void);
asmlinkage extern void handle_adel(void);
asmlinkage extern void handle_ades(void);
asmlinkage extern void handle_ibe(void);
asmlinkage extern void handle_dbe(void);
asmlinkage extern void handle_sys(void);
asmlinkage extern void handle_bp(void);
asmlinkage extern void handle_ri(void);
asmlinkage extern void handle_cpu(void);
asmlinkage extern void handle_ov(void);
asmlinkage extern void handle_tr(void);
asmlinkage extern void handle_reserved(void);
asmlinkage extern void handle_fpe(void);

void die_if_kernel(char * str, struct pt_regs * regs, long err)
{
	int i;
	unsigned long *sp, *pc;

	if (regs->cp0_status & (ST0_ERL|ST0_EXL) == 0)
		return;

	sp = (unsigned long *)regs->reg29;
	pc = (unsigned long *)regs->cp0_epc;

	console_verbose();
	printk("%s: %08lx\n", str, err );

	/*
	 * Saved main processor registers
	 */
	printk("at   : %08lx\n", regs->reg1);
	printk("v0   : %08lx %08lx\n", regs->reg2, regs->reg3);
	printk("a0   : %08lx %08lx %08lx %08lx\n",
	       regs->reg4, regs->reg5, regs->reg6, regs->reg7);
	printk("t0   : %08lx %08lx %08lx %08lx %08lx\n",
	       regs->reg8, regs->reg9, regs->reg10, regs->reg11, regs->reg12);
	printk("t5   : %08lx %08lx %08lx %08lx %08lx\n",
	       regs->reg13, regs->reg14, regs->reg15, regs->reg24, regs->reg25);
	printk("s0   : %08lx %08lx %08lx %08lx\n",
	       regs->reg16, regs->reg17, regs->reg18, regs->reg19);
	printk("s4   : %08lx %08lx %08lx %08lx\n",
	       regs->reg20, regs->reg21, regs->reg22, regs->reg23);
	printk("gp   : %08lx\n", regs->reg28);
	printk("sp   : %08lx\n", regs->reg29);
	printk("fp/s8: %08lx\n", regs->reg30);
	printk("ra   : %08lx\n", regs->reg31);

	/*
	 * Saved cp0 registers
	 */
	printk("EPC  : %08lx\nErrorEPC: %08lx\nStatus: %08lx\n",
	        regs->cp0_epc, regs->cp0_errorepc, regs->cp0_status);
	/*
	 * Some goodies...
	 */
	printk("Int  : %ld\n", regs->interrupt);

	/*
	 * Dump the stack
	 */
	if (STACK_MAGIC != *(unsigned long *)current->kernel_stack_page)
		printk("Corrupted stack page\n");
	printk("Process %s (pid: %d, process nr: %d, stackpage=%08lx)\nStack: ",
		current->comm, current->pid, 0xffff & i,
	        current->kernel_stack_page);
	for(i=0;i<5;i++)
		printk("%08lx ", *sp++);

	printk("\nCode: ");
	for(i=0;i<5;i++)
		printk("%08lx ", *pc++);
	printk("\n");
	do_exit(SIGSEGV);
}

void trap_init(void)
{
	int i;

#if 0
	set_except_vector(0, handle_int);
	set_except_vector(1, handle_mod);
	set_except_vector(2, handle_tlbl);
	set_except_vector(3, handle_tlbs);
	set_except_vector(4, handle_adel);
	set_except_vector(5, handle_ades);
	set_except_vector(6, handle_ibe);
	set_except_vector(7, handle_dbe);
	set_except_vector(8, handle_sys);
	set_except_vector(9, handle_bp);
	set_except_vector(10, handle_ri);
	set_except_vector(11, handle_cpu);
	set_except_vector(12, handle_ov);
	set_except_vector(13, handle_tr);
	set_except_vector(14, handle_reserved);
	set_except_vector(15, handle_fpe);

	for (i=16;i<256;i++)
		set_except_vector(i, handle_reserved);
#endif
}
