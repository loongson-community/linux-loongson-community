/*
 *  linux/arch/mips/mips3/showregs.c
 *
 *  Copyright (C) 1995, 1996 Ralf Baechle
 */
#include <linux/kernel.h>
#include <linux/ptrace.h>

void show_regs(struct pt_regs * regs)
{
	/*
	 * Saved main processor registers
	 */
	printk("$0 : %016Lx %016Lx %016Lx %016Lx\n",
	       0ULL, regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("$4 : %016Lx %016Lx %016Lx %016Lx\n",
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8 : %016Lx %016Lx %016Lx %016Lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11]);
	printk("$12: %016Lx %016Lx %016Lx %016Lx\n",
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16: %016Lx %016Lx %016Lx %016Lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19]);
	printk("$20: %016Lx %016Lx %016Lx %016Lx\n",
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24: %016Lx %016Lx\n",
	       regs->regs[24], regs->regs[25]);
	printk("$28: %016Lx %016Lx %016Lx %016Lx\n",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31]);

	/*
	 * Saved cp0 registers
	 */
	printk("epc   : %016Lx\nStatus: %08x\nCause : %08x\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause);
}
