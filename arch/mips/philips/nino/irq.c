/*
 * irq.c: Fine grained interrupt handling for Nino
 *
 * Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 */
#include <linux/init.h>

#include <linux/errno.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/system.h>

#include <asm/ptrace.h>
#include <asm/processor.h>
#include <asm/tx3912.h>

/*
 * Linux has a controller-independent x86 interrupt architecture.
 * every controller has a 'controller-template', that is used
 * by the main code to do the right thing. Each driver-visible
 * interrupt source is transparently wired to the apropriate
 * controller. Thus drivers need not be aware of the
 * interrupt-controller.
 *
 * Various interrupt controllers we handle: 8259 PIC, SMP IO-APIC,
 * PIIX4's internal 8259 PIC and SGI's Visual Workstation Cobalt (IO-)APIC.
 * (IO-APICs assumed to be messaging to Pentium local-APICs)
 *
 * the code is designed to be easily extended with new/different
 * interrupt controllers, without having to do assembly magic.
 */

extern asmlinkage void ninoIRQ(void);
extern asmlinkage void do_IRQ(int irq, struct pt_regs *regs);
extern void init_generic_irq(void);

static void enable_irq4(unsigned int irq)
{
	unsigned long flags;

	save_and_cli(flags);
	if(irq == 0) {
		IntEnable5 |= INT5_PERIODICINT;
		IntEnable6 |= INT6_PERIODICINT;
	}
	restore_flags(flags);
}

static unsigned int startup_irq4(unsigned int irq)
{
	enable_irq4(irq);

	return 0;		/* Never anything pending  */
}

static void disable_irq4(unsigned int irq)
{
	unsigned long flags;

	save_and_cli(flags);
	if(irq == 0) {
                IntEnable6 &= ~INT6_PERIODICINT;
	        IntClear5 |= INT5_PERIODICINT;
                IntClear6 |= INT6_PERIODICINT;
	}
	restore_flags(flags);
}

#define shutdown_irq4		disable_irq4
#define mask_and_ack_irq4	disable_irq4

static void end_irq4(unsigned int irq)
{
	if(!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_irq4(irq);
}

static struct hw_interrupt_type irq4_type = {
	"IRQ4",
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

	if(IntStatus6 & INT6_PERIODICINT) {
		irq = 0;
		goto done;
	}

	/* if irq == -1, then the interrupt has already been cleared */
	if(irq == -1) {
		printk("IRQ6 Status Register = 0x%08x\n", IntStatus6);
		goto end;
	}

done:
	do_IRQ(irq, regs);

end:	
	return;
}

static void enable_irq2(unsigned int irq)
{
	unsigned long flags;

	save_and_cli(flags);
	if(irq == 2 || irq == 3) {
		IntEnable1 = 0x00000000;
		IntEnable2 = 0xfffff000;
		IntEnable3 = 0x00000000;
		IntEnable4 = 0x00000000;
		IntClear1 = 0xffffffff;
		IntClear2 = 0xffffffff;
		IntClear3 = 0xffffffff;
		IntClear4 = 0xffffffff;
		IntClear5 = 0xffffffff;
	}
	restore_flags(flags);
}

static unsigned int startup_irq2(unsigned int irq)
{
	enable_irq2(irq);

	return 0;		/* Never anything pending  */
}

static void disable_irq2(unsigned int irq)
{
	unsigned long flags;

	save_and_cli(flags);
	IntEnable1 = 0x00000000;
	IntEnable2 = 0x00000000;
	IntEnable3 = 0x00000000;
	IntEnable4 = 0x00000000;
	IntClear1 = 0xffffffff;
	IntClear2 = 0xffffffff;
	IntClear3 = 0xffffffff;
	IntClear4 = 0xffffffff;
	IntClear5 = 0xffffffff;
	restore_flags(flags);
}

#define shutdown_irq2		disable_irq2
#define mask_and_ack_irq2	disable_irq2

static void end_irq2(unsigned int irq)
{
	if(!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_irq2(irq);
}

static struct hw_interrupt_type irq2_type = {
	"IRQ2",
	startup_irq2,
	shutdown_irq2,
	enable_irq2,
	disable_irq2,
	mask_and_ack_irq2,
	end_irq2,
	NULL
};

void irq2_dispatch(struct pt_regs *regs)
{
	int irq = -1;

	if(IntStatus2 & 0xfffff000) {
		irq = 2;
		goto done;
	}
	if(IntStatus2 & 0xfffff000) {
		irq = 3;
		goto done;
	}

	/* if irq == -1, then the interrupt has already been cleared */
	if (irq == -1) {
		IntClear1 = 0xffffffff;
		IntClear3 = 0xffffffff;
		IntClear4 = 0xffffffff;
		IntClear5 = 0xffffffff;
		goto end;
	}

done:
	do_IRQ(irq, regs);

end:	
	return;
}

void irq_bad(struct pt_regs *regs)
{
	/* This should never happen */
	printk("Invalid interrupt, spinning...\n");
	printk(" CAUSE register = 0x%08lx\n", regs->cp0_cause);
	printk("STATUS register = 0x%08lx\n", regs->cp0_status);
	printk("   EPC register = 0x%08lx\n", regs->cp0_epc);
	while(1);
}

void __init nino_irq_setup(void)
{
	unsigned int i;

	/* Disable interrupts */
	IntEnable1 = 0x00000000;
	IntEnable2 = 0x00000000;
	IntEnable3 = 0x00000000;
	IntEnable4 = 0x00000000;
	IntEnable5 = 0x00000000;
	IntEnable6 = 0x00000000;

	/* Clear interrupts */
	IntClear1 = 0xffffffff;
	IntClear2 = 0xffffffff;
	IntClear3 = 0xffffffff;
	IntClear4 = 0xffffffff;
	IntClear5 = 0xffffffff;

	/* Change location of exception vector table */
	change_cp0_status(ST0_BEV, 0);

	/* Initialize IRQ vector table */
	init_generic_irq();

	/* Initialize hardware IRQ structure */
	for (i = 0; i < 16; i++) {
		hw_irq_controller *handler = NULL;
		if (i == 0)
			handler		= &irq4_type;
		else if (i > 1 && i < 4)
			handler		= &irq2_type;
		else
			handler		= NULL;

		irq_desc[i].status	= IRQ_DISABLED;
		irq_desc[i].action	= 0;
		irq_desc[i].depth	= 1;
		irq_desc[i].handler	= handler;
	}

	/* Set up the external interrupt exception vector */
	set_except_vector(0, ninoIRQ);
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
