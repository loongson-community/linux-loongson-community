/*
 * ip22-berr.c: Bus error handling.
 *
 * Copyright (C) 2002 Ladislav Michl
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/addrspace.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/branch.h>
#include <asm/sgi/sgimc.h>
#include <asm/sgi/sgihpc.h>

unsigned int cpu_err_stat;	/* Status reg for CPU */
unsigned int gio_err_stat;	/* Status reg for GIO */
unsigned int cpu_err_addr;	/* Error address reg for CPU */
unsigned int gio_err_addr;	/* Error address reg for GIO */

volatile int nofault;

static void save_and_clear_buserr(void)
{
	/* save memory controler's error status registers */ 
	cpu_err_addr = mcmisc_regs->cerr;
	cpu_err_stat = mcmisc_regs->cstat;
	gio_err_addr = mcmisc_regs->gerr;
	gio_err_stat = mcmisc_regs->gstat;

	mcmisc_regs->cstat = mcmisc_regs->gstat = 0;
}

/*
 * MC sends an interrupt whenever bus or parity errors occur. In addition, 
 * if the error happened during a CPU read, it also asserts the bus error 
 * pin on the R4K. Code in bus error handler save the MC bus error registers
 * and then clear the interrupt when this happens.
 */

void be_ip22_interrupt(int irq, struct pt_regs *regs)
{
	save_and_clear_buserr();
	printk(KERN_ALERT "Bus error, epc == %08lx, ra == %08lx\n",
	       regs->cp0_epc, regs->regs[31]);
	die_if_kernel("Oops", regs);
	force_sig(SIGBUS, current);
}

int be_ip22_handler(struct pt_regs *regs, int is_fixup)
{
	save_and_clear_buserr();
	if (nofault) {
		nofault = 0;
		compute_return_epc(regs);
		return MIPS_BE_DISCARD;
	}
	return MIPS_BE_FIXUP;
}

int ip22_baddr(unsigned int *val, unsigned long addr)
{
	nofault = 1;
	*val = *(volatile unsigned int *) addr;
	__asm__ __volatile__("nop;nop;nop;nop");
	if (nofault) {
		nofault = 0;
		return 0;
	}
	return -EFAULT;
}

void __init bus_error_init(void)
{
	be_board_handler = be_ip22_handler;
}
