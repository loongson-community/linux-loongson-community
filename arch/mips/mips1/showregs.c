/*
 *  linux/arch/mips/mips1/showregs.c
 *
 *  Copyright (C) 1995, 1996 Ralf Baechle, Paul M. Antoine.
 */
#include <linux/kernel.h>
#include <linux/ptrace.h>

void show_regs(struct pt_regs * regs)
{
	/*
	 * Saved main processor registers
	 */
	printk("$0 : %08x %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       0, regs->regs[1], regs->regs[2], regs->regs[3],
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8 : %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11],
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19],
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24: %08lx %08lx                   %08lx %08lx %08lx %08lx\n",
	       regs->regs[24], regs->regs[25], regs->regs[28], regs->regs[29],
               regs->regs[30], regs->regs[31]);

	/*
	 * Saved cp0 registers
	 */
	printk("epc  : %08lx\nStatus: %08x\nCause : %08x\nBadVAdddr : %08x\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause,regs->cp0_badvaddr);
}
