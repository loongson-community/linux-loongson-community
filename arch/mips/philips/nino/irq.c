/*
 *  arch/mips/philips/nino/irq.c
 *
 *  Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Interrupt service routines for Philips Nino
 */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/tx3912.h>

extern asmlinkage void do_IRQ(int irq, struct pt_regs *regs);

static void enable_irq6(unsigned int irq)
{
	if(irq == 0) {
		IntEnable5 |= INT5_PERIODICINT;
		IntEnable6 |= INT6_PERIODICINT;
	}
}

static unsigned int startup_irq6(unsigned int irq)
{
	enable_irq6(irq);

	return 0;		/* Never anything pending  */
}

static void disable_irq6(unsigned int irq)
{
	if(irq == 0) {
		IntEnable6 &= ~INT6_PERIODICINT;
		IntClear5 |= INT5_PERIODICINT;
		IntClear6 |= INT6_PERIODICINT;
	}
}

#define shutdown_irq6		disable_irq6
#define mask_and_ack_irq6	disable_irq6

static void end_irq6(unsigned int irq)
{
	if(!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_irq6(irq);
}

static struct hw_interrupt_type irq6_type = {
	"MIPS",
	startup_irq6,
	shutdown_irq6,
	enable_irq6,
	disable_irq6,
	mask_and_ack_irq6,
	end_irq6,
	NULL
};

void irq6_dispatch(struct pt_regs *regs)
{
	int irq = -1;

	if(IntStatus6 & INT6_PERIODICINT) {
		irq = 0;
		goto done;
	}

	/* if irq == -1, then the interrupt has already been cleared */
	if(irq == -1) {
		panic("No handler installed for MIPS IRQ6\n");
	}

done:
	do_IRQ(irq, regs);

end:	
	return;
}

static void enable_irq4(unsigned int irq)
{
	set_cp0_status(STATUSF_IP4);
	if(irq == 2) {
		IntClear2 = 0xffffffff;
		IntEnable2 |= 0x07c00000;
	}
}

static unsigned int startup_irq4(unsigned int irq)
{
	enable_irq4(irq);

	return 0;		/* Never anything pending  */
}

static void disable_irq4(unsigned int irq)
{
	clear_cp0_status(STATUSF_IP4);
}

#define shutdown_irq4		disable_irq4
#define mask_and_ack_irq4	disable_irq4

static void end_irq4(unsigned int irq)
{
	if(!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_irq4(irq);
}

static struct hw_interrupt_type irq4_type = {
	"MIPS",
	startup_irq4,
	shutdown_irq4,
	enable_irq4,
	disable_irq4,
	mask_and_ack_irq4,
	end_irq4,
	NULL
};

void irq4_dispatch(struct pt_regs *regs)
{
	int irq = -1;

	if(IntStatus2 & 0x07c00000) {
		irq = 2;
		goto done;
	}

	/* if irq == -1, then the interrupt has already been cleared */
	if (irq == -1) {
		panic("No handler installed for MIPS IRQ4\n");
	}

done:
	do_IRQ(irq, regs);

end:	
	return;
}

void irq_bad(struct pt_regs *regs)
{
	/* This should never happen */
	printk("Stray interrupt, spinning...\n");
	printk(" CAUSE register = 0x%08lx\n", regs->cp0_cause);
	printk("STATUS register = 0x%08lx\n", regs->cp0_status);
	printk("   EPC register = 0x%08lx\n", regs->cp0_epc);
	while(1);
}

void __init nino_irq_setup(void)
{
	extern asmlinkage void ninoIRQ(void);
	extern void init_generic_irq(void);

	unsigned int i;

	/* Disable all hardware interrupts */
	change_cp0_status(ST0_IM, 0x00);

	/* Clear any pending interrupts */
	IntClear1 = 0xffffffff;
	IntClear2 = 0xffffffff;
	IntClear3 = 0xffffffff;
	IntClear4 = 0xffffffff;
	IntClear5 = 0xffffffff;

	/* FIXME: disable interrupts 1,3,4 */
	IntEnable1 = 0x00000000;
	IntEnable2 = 0xfffff000;
	IntEnable3 = 0x00000000;
	IntEnable4 = 0x00000000;
	IntEnable5 = 0xffffffff;

	/* Initialize IRQ vector table */
	init_generic_irq();

	/* Initialize IRQ action handlers */
	for (i = 0; i < 16; i++) {
		hw_irq_controller *handler = NULL;
		if (i == 0 || i == 3)
			handler		= &irq6_type;
		else if (i == 2)
			handler		= &irq4_type;
		else
			handler		= NULL;

		irq_desc[i].status	= IRQ_DISABLED;
		irq_desc[i].action	= 0;
		irq_desc[i].depth	= 1;
		irq_desc[i].handler	= handler;
	}

	/* Set up the external interrupt exception vector */
	set_except_vector(0, ninoIRQ);

	/* Enable high priority interrupts */
	IntEnable6 = (INT6_GLOBALEN | 0xffff);

	/* Enable all interrupts */
	change_cp0_status(ST0_IM, ALLINTS);
}

void (*irq_setup)(void);

void __init init_IRQ(void)
{
#ifdef CONFIG_REMOTE_DEBUG
	extern void breakpoint(void);
	extern void set_debug_traps(void);

	printk("Wait for gdb client connection ...\n");
	set_debug_traps();
	breakpoint();
#endif

	/* Invoke board-specific irq setup */
	irq_setup();
}
