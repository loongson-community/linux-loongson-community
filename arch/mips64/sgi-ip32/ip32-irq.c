/*
 * Code to handle IP32 IRQs
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 Harald Koerfgen
 * Copyright (C) 2001 Keith M Wesolowski
 */
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/bitops.h>

#include <asm/bitops.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/ip32/ip32_ints.h>
#include <asm/ip32/crime.h>
#include <asm/ip32/mace.h>
#include <asm/signal.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/sched.h>

#undef DEBUG_IRQ
#ifdef DEBUG_IRQ
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

/* O2 irq map
 *
 * IP0 -> software (ignored)
 * IP1 -> software (ignored)
 * IP2 -> (irq0) C crime 1.1 all interrupts; crime 1.5 ???
 * IP3 -> (irq1) X unknown
 * IP4 -> (irq2) X unknown
 * IP5 -> (irq3) X unknown
 * IP6 -> (irq4) X unknown
 * IP7 -> (irq5) 0 CPU count/compare timer (system timer)
 *
 * crime: (C)
 *
 * CRIME_INT_STAT 31:0:
 *
 * 0 -> 1 Video in 1
 * 1 -> 2 Video in 2
 * 2 -> 3 Video out
 * 3 -> 4 Mace ethernet
 * 4 -> S  SuperIO sub-interrupt
 * 5 -> M  Miscellaneous sub-interrupt
 * 6 -> A  Audio sub-interrupt
 * 7 -> 8  PCI bridge errors
 * 8 -> 9  PCI SCSI aic7xxx 0
 * 9 -> 10  PCI SCSI aic7xxx 1
 * 10 -> 11 PCI slot 0
 * 11 -> 12 unused (PCI slot 1)
 * 12 -> 13 unused (PCI slot 2)
 * 13 -> 14 unused (PCI shared 0)
 * 14 -> 15 unused (PCI shared 1)
 * 15 -> 16 unused (PCI shared 2)
 * 16 -> 17 GBE0 (E)
 * 17 -> 18 GBE1 (E)
 * 18 -> 19 GBE2 (E)
 * 19 -> 20 GBE3 (E)
 * 20 -> 21 CPU errors
 * 21 -> 22 Memory errors
 * 22 -> 23 RE empty edge (E)
 * 23 -> 24 RE full edge (E)
 * 24 -> 25 RE idle edge (E)
 * 25 -> 26 RE empty level
 * 26 -> 27 RE full level
 * 27 -> 28 RE idle level
 * 28 -> 29  unused (software 0) (E)
 * 29 -> 30  unused (software 1) (E)
 * 30 -> 31  unused (software 2) - crime 1.5 CPU SysCorError (E)
 * 31 -> 32 VICE
 *
 * S, M, A: Use the MACE ISA interrupt register
 * MACE_ISA_INT_STAT 31:0
 *
 * 0-7 -> 33-40 Audio
 * 8 -> 41 RTC
 * 9 -> 42 Keyboard
 * 10 -> X Keyboard polled
 * 11 -> 44 Mouse
 * 12 -> X Mouse polled
 * 13-15 -> 46-48 Count/compare timers
 * 16-19 -> 49-52 Parallel (16 E)
 * 20-25 -> 53-58 Serial 1 (22 E)
 * 26-31 -> 59-64 Serial 2 (28 E)
 *
 * Note that this means IRQs 5-7, 43, and 45 do not exist.  This is a
 * different IRQ map than IRIX uses, but that's OK as Linux irq handling
 * is quite different anyway.
 */

/* Some initial interrupts to set up */
extern void crime_memerr_intr (unsigned int irq, void *dev_id,
			       struct pt_regs *regs);
extern void crime_cpuerr_intr (unsigned int irq, void *dev_id,
			       struct pt_regs *regs);

struct irqaction memerr_irq = { crime_memerr_intr, SA_INTERRUPT,
			       0, "CRIME memory error", NULL,
			       NULL };
struct irqaction cpuerr_irq = { crime_cpuerr_intr, SA_INTERRUPT,
			       0, "CRIME CPU error", NULL,
			       NULL };

extern void ip32_handle_int (void);
asmlinkage unsigned int do_IRQ(int irq, struct pt_regs *regs);

static void enable_none(unsigned int irq) { }
static unsigned int startup_none(unsigned int irq) { return 0; }
static void disable_none(unsigned int irq) { }
static void ack_none(unsigned int irq)
{
	/*
	 * 'what should we do if we get a hw irq event on an illegal vector'.
	 * each architecture has to answer this themselves, it doesnt deserve
	 * a generic callback i think.
	 */
	printk("unexpected interrupt %d\n", irq);
}

/* startup is the same as "enable", shutdown is same as "disable" */
#define shutdown_none	disable_none
#define end_none	enable_none

struct hw_interrupt_type no_irq_type = {
	"none",
	startup_none,
	shutdown_none,
	enable_none,
	disable_none,
	ack_none,
	end_none
};

/*
 * Controller mappings for all interrupt sources:
 */

irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned =
	{ [0 ... NR_IRQS-1] = { 0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};


/* For interrupts wired from a single device to the CPU.  Only the clock
 * uses this it seems, which is IRQ 0 and IP7.
 */

static void enable_cpu_irq(unsigned int irq)
{
	set_cp0_status(STATUSF_IP7);
}

static unsigned int startup_cpu_irq(unsigned int irq)
{
	enable_cpu_irq(irq);
	return 0;
}

static void disable_cpu_irq(unsigned int irq)
{
	clear_cp0_status(STATUSF_IP7);
}

static void end_cpu_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_cpu_irq (irq);
}

#define shutdown_cpu_irq disable_cpu_irq
#define mask_and_ack_cpu_irq disable_cpu_irq

static struct hw_interrupt_type ip32_cpu_interrupt = {
	"IP32 CPU",
	startup_cpu_irq,
	shutdown_cpu_irq,
	enable_cpu_irq,
	disable_cpu_irq,
	mask_and_ack_cpu_irq,
	end_cpu_irq,
	NULL
};

/*
 * This is for pure CRIME interrupts - ie not MACE.  The advantage?
 * We get to split the register in half and do faster lookups.
 */

static void enable_crime_irq(unsigned int irq)
{
	u64 crime_mask;
	unsigned long flags;

	save_and_cli(flags);
	crime_mask = crime_read_64(CRIME_INT_MASK);
	crime_mask |= 1 << (irq - 1);
	crime_write_64(CRIME_INT_MASK, crime_mask);
	restore_flags(flags);
}

static unsigned int startup_crime_irq(unsigned int irq)
{
	enable_crime_irq(irq);
	return 0; /* This is probably not right; we could have pending irqs */
}

static void disable_crime_irq(unsigned int irq)
{
	u64 crime_mask;
	unsigned long flags;

	save_and_cli(flags);
	crime_mask = crime_read_64(CRIME_INT_MASK);
	crime_mask &= ~(1 << (irq - 1));
	crime_write_64(CRIME_INT_MASK, crime_mask);
	restore_flags(flags);
}

static void mask_and_ack_crime_irq (unsigned int irq)
{
	u64 crime_mask;
	unsigned long flags;

	/* Edge triggered interrupts must be cleared. */
	if ((irq <= CRIME_GBE0_IRQ && irq >= CRIME_GBE3_IRQ)
	    || (irq <= CRIME_RE_EMPTY_E_IRQ && irq >= CRIME_RE_IDLE_E_IRQ)
	    || (irq <= CRIME_SOFT0_IRQ && irq >= CRIME_SOFT2_IRQ)) {
		save_and_cli(flags);
		crime_mask = crime_read_64(CRIME_HARD_INT);
		crime_mask &= ~(1 << (irq - 1));
		crime_write_64(CRIME_HARD_INT, crime_mask);
		restore_flags(flags);
	}
	disable_crime_irq(irq);
}

static void end_crime_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_crime_irq (irq);
}

#define shutdown_crime_irq disable_crime_irq

static struct hw_interrupt_type ip32_crime_interrupt = {
	"IP32 CRIME",
	startup_crime_irq,
	shutdown_crime_irq,
	enable_crime_irq,
	disable_crime_irq,
	mask_and_ack_crime_irq,
	end_crime_irq,
	NULL
};

/*
 * This is for MACE PCI interrupts.  We can decrease bus traffic by masking
 * as close to the source as possible.  This also means we can take the
 * next chunk of the CRIME register in one piece.
 */

static void enable_macepci_irq(unsigned int irq)
{
	u32 mace_mask;
	u64 crime_mask;
	unsigned long flags;

	save_and_cli(flags);
	mace_mask = mace_read_32(MACEPCI_CONTROL);
	mace_mask |= MACEPCI_CONTROL_INT(irq - 9);
	mace_write_32(MACEPCI_CONTROL, mace_mask);
	/* In case the CRIME interrupt isn't enabled, we must enable it;
	 * however, we never disable interrupts at that level.
	 */
	crime_mask = crime_read_64(CRIME_INT_MASK);
	crime_mask |= 1 << (irq - 1);
	crime_write_64(CRIME_INT_MASK, crime_mask);
	restore_flags(flags);
}

static unsigned int startup_macepci_irq(unsigned int irq)
{
	enable_macepci_irq (irq);
	return 0; /* XXX */
}

static void disable_macepci_irq(unsigned int irq)
{
	u32 mace_mask;
	unsigned long flags;

	save_and_cli(flags);
	mace_mask = mace_read_32(MACEPCI_CONTROL);
	mace_mask &= ~MACEPCI_CONTROL_INT(irq - 9);
	mace_write_32(MACEPCI_CONTROL, mace_mask);
	restore_flags(flags);
}

static void end_macepci_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_macepci_irq (irq);
}

#define shutdown_macepci_irq disable_macepci_irq
#define mask_and_ack_macepci_irq disable_macepci_irq

static struct hw_interrupt_type ip32_macepci_interrupt = {
	"IP32 MACE PCI",
	startup_macepci_irq,
	shutdown_macepci_irq,
	enable_macepci_irq,
	disable_macepci_irq,
	mask_and_ack_macepci_irq,
	end_macepci_irq,
	NULL
};

/* This is used for MACE ISA interrupts.  That means bits 4-6 in the
 * CRIME register.
 */

static void enable_maceisa_irq (unsigned int irq)
{
	u64 crime_mask;
	u32 mace_mask;
	unsigned int crime_int = 0;
	unsigned long flags;

	DBG ("maceisa enable: %u\n", irq);
	
	switch (irq) {
	case MACEISA_AUDIO_SW_IRQ ... MACEISA_AUDIO3_MERR_IRQ:
		crime_int = MACE_AUDIO_INT;
		break;
	case MACEISA_RTC_IRQ ... MACEISA_TIMER2_IRQ:
		crime_int = MACE_MISC_INT;
		break;
	case MACEISA_PARALLEL_IRQ ... MACEISA_SERIAL2_RDMAOR_IRQ:
		crime_int = MACE_SUPERIO_INT;
		break;
	}
	DBG ("crime_int %016lx enabled\n", crime_int);
	save_and_cli(flags);
	crime_mask = crime_read_64(CRIME_INT_MASK);
	crime_mask |= crime_int;
	crime_write_64(CRIME_INT_MASK, crime_mask);
	mace_mask = mace_read_32(MACEISA_INT_MASK);
	mace_mask |= 1 << (irq - 33);
	mace_write_32(MACEISA_INT_MASK, mace_mask);
	restore_flags(flags);
}

static unsigned int startup_maceisa_irq (unsigned int irq)
{
	enable_maceisa_irq(irq);
	return 0;
}

static void disable_maceisa_irq(unsigned int irq)
{
	u32 mace_mask;
	unsigned long flags;

	save_and_cli (flags);
	mace_mask = mace_read_32(MACEISA_INT_MASK);
	mace_mask &= ~(1 << (irq - 33));
	mace_write_32(MACEISA_INT_MASK, mace_mask);
	restore_flags(flags);
}

static void mask_and_ack_maceisa_irq(unsigned int irq)
{
	u32 mace_mask;
	unsigned long flags;

	switch (irq) {
	case MACEISA_PARALLEL_IRQ:
	case MACEISA_SERIAL1_TDMAPR_IRQ:
	case MACEISA_SERIAL2_TDMAPR_IRQ:
		save_and_cli(flags);
		mace_mask = mace_read_32(MACEISA_INT_STAT);
		mace_mask &= ~(1 << (irq - 33));
		mace_write_32(MACEISA_INT_STAT, mace_mask);
		restore_flags(flags);
		break;
	}
	disable_maceisa_irq(irq);
}

static void end_maceisa_irq(unsigned irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_maceisa_irq (irq);
}

#define shutdown_maceisa_irq disable_maceisa_irq

static struct hw_interrupt_type ip32_maceisa_interrupt = {
	"IP32 MACE ISA",
	startup_maceisa_irq,
	shutdown_maceisa_irq,
	enable_maceisa_irq,
	disable_maceisa_irq,
	mask_and_ack_maceisa_irq,
	end_maceisa_irq,
	NULL
};

/* This is used for regular non-ISA, non-PCI MACE interrupts.  That means
 * bits 0-3 and 7 in the CRIME register.
 */

static void enable_mace_irq(unsigned int irq)
{
	u64 crime_mask;
	unsigned long flags;

	save_and_cli (flags);
	crime_mask = crime_read_64 (CRIME_INT_MASK);
	crime_mask |= 1 << (irq - 1);
	crime_write_64 (CRIME_INT_MASK, crime_mask);
	restore_flags (flags);
}

static unsigned int startup_mace_irq(unsigned int irq)
{
	enable_mace_irq(irq);
	return 0;
}

static void disable_mace_irq(unsigned int irq)
{
	u64 crime_mask;
	unsigned long flags;

	save_and_cli (flags);
	crime_mask = crime_read_64 (CRIME_INT_MASK);
	crime_mask &= ~(1 << (irq - 1));
	crime_write_64 (CRIME_INT_MASK, crime_mask);
	restore_flags(flags);
}

static void end_mace_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_mace_irq (irq);
}

#define shutdown_mace_irq disable_mace_irq
#define mask_and_ack_mace_irq disable_mace_irq

static struct hw_interrupt_type ip32_mace_interrupt = {
	"IP32 MACE",
	startup_mace_irq,
	shutdown_mace_irq,
	enable_mace_irq,
	disable_mace_irq,
	mask_and_ack_mace_irq,
	end_mace_irq,
	NULL
};

static void ip32_unknown_interrupt (struct pt_regs *regs)
{
	u64 crime;
	u32 mace;
	
	printk ("Unknown interrupt occurred!\n");
	printk ("cp0_status: %08x\tcp0_cause: %08x\n",
		read_32bit_cp0_register (CP0_STATUS),
		read_32bit_cp0_register (CP0_CAUSE));
	crime = crime_read_64 (CRIME_INT_MASK);
	printk ("CRIME interrupt mask: %016lx\n", crime);
	crime = crime_read_64 (CRIME_INT_STAT);
	printk ("CRIME interrupt status: %016lx\n", crime);
	crime = crime_read_64 (CRIME_HARD_INT);
	printk ("CRIME hardware interrupt register: %016lx\n", crime);
	mace = mace_read_32 (MACEISA_INT_MASK);
	printk ("MACE ISA interrupt mask: %08x\n", mace);
	mace = mace_read_32 (MACEISA_INT_STAT);
	printk ("MACE ISA interrupt status: %08x\n", mace);
	mace = mace_read_32 (MACEPCI_CONTROL);
	printk ("MACE PCI control register: %08x\n", mace);

	printk("Register dump:\n");
	show_regs(regs);

	printk("Please mail this report to linux-mips@oss.sgi.com\n");
	printk("Spinning...");
	while(1) ;
}

void __init init_IRQ(void)
{
	unsigned int irq;
	int i;

	/* Install our interrupt handler, then clear and disable all
	 * CRIME and MACE interrupts.
	 */
	crime_write_64(CRIME_INT_MASK, 0);
	crime_write_64(CRIME_HARD_INT, 0);
	crime_write_64(CRIME_SOFT_INT, 0);
	mace_write_32(MACEISA_INT_STAT, 0);
	mace_write_32(MACEISA_INT_MASK, 0);
	set_except_vector(0, ip32_handle_int);

	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status  = IRQ_DISABLED;
		irq_desc[i].action  = NULL;
		irq_desc[i].depth   = 1;
		irq_desc[i].handler = &no_irq_type;
	}

	for (irq = 0; irq <= IP32_IRQ_MAX; irq++) {
		hw_irq_controller *controller;

		if (irq == CLOCK_IRQ)
			controller = &ip32_cpu_interrupt;
		else if (irq <= MACE_PCI_BRIDGE_IRQ && irq >= MACE_VID_IN1_IRQ)
			controller = &ip32_mace_interrupt;
		else if (irq <= MACEPCI_SHARED2_IRQ && irq >= MACEPCI_SCSI0_IRQ)
			controller = &ip32_macepci_interrupt;
		else if (irq <= CRIME_VICE_IRQ && irq >= CRIME_GBE0_IRQ)
			controller = &ip32_crime_interrupt;
		else
			controller = &ip32_maceisa_interrupt;

		irq_desc[irq].status = IRQ_DISABLED;
		irq_desc[irq].action = 0;
		irq_desc[irq].depth = 0;
		irq_desc[irq].handler = controller;
	}
	setup_irq(CRIME_MEMERR_IRQ, &memerr_irq);
	setup_irq(CRIME_CPUERR_IRQ, &cpuerr_irq);
}

/* CRIME 1.1 appears to deliver all interrupts to this one pin. */
void ip32_irq0(struct pt_regs *regs)
{
	u64 crime_int = crime_read_64 (CRIME_INT_STAT);
	int irq = 0;
	
	if (crime_int & CRIME_MACE_INT_MASK) {
		crime_int &= CRIME_MACE_INT_MASK;
		irq = ffs (crime_int);
	} else if (crime_int & CRIME_MACEISA_INT_MASK) {
		u32 mace_int;
		mace_int = mace_read_32 (MACEISA_INT_STAT);
		if (mace_int == 0)
			irq = 0;
		else
			irq = ffs (mace_int) + 32;
	} else if (crime_int & CRIME_MACEPCI_INT_MASK) {
		crime_int &= CRIME_MACEPCI_INT_MASK;
		crime_int >>= 8;
		irq = ffs (crime_int) + 8;
	} else if (crime_int & 0xffff0000) {
		crime_int >>= 16;
		irq = ffs (crime_int) + 16;
	}
	if (irq == 0)
		ip32_unknown_interrupt(regs);
	DBG("*irq %u*\n", irq);
	do_IRQ(irq, regs);
}

void ip32_irq1(struct pt_regs *regs)
{
	ip32_unknown_interrupt (regs);
}

void ip32_irq2(struct pt_regs *regs)
{
	ip32_unknown_interrupt (regs);
}

void ip32_irq3(struct pt_regs *regs)
{
	ip32_unknown_interrupt (regs);
}

void ip32_irq4(struct pt_regs *regs)
{
	ip32_unknown_interrupt (regs);
}

void ip32_irq5(struct pt_regs *regs)
{
	do_IRQ (CLOCK_IRQ, regs);
}

/*
 * Generic, controller-independent functions:
 */

/* this was setup_x86_irq but it seems pretty generic */
int setup_irq(unsigned int irq, struct irqaction * new)
{
	int shared = 0;
	unsigned long flags;
	struct irqaction *old, **p;
	irq_desc_t *desc = irq_desc + irq;

	/*
	 * Some drivers like serial.c use request_irq() heavily,
	 * so we have to be careful not to interfere with a
	 * running system.
	 */
	if (new->flags & SA_SAMPLE_RANDOM) {
		/*
		 * This function might sleep, we want to call it first,
		 * outside of the atomic block.
		 * Yes, this might clear the entropy pool if the wrong
		 * driver is attempted to be loaded, without actually
		 * installing a new handler, but is this really a problem,
		 * only the sysadmin is able to do this.
		 */
		rand_initialize_irq(irq);
	}

	/*
	 * The following block of code has to be executed atomically
	 */
	spin_lock_irqsave(&desc->lock,flags);
	p = &desc->action;
	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & new->flags & SA_SHIRQ)) {
			spin_unlock_irqrestore(&desc->lock,flags);
			return -EBUSY;
		}

		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
		shared = 1;
	}

	*p = new;

	if (!shared) {
		desc->depth = 0;
		desc->status &= ~(IRQ_DISABLED | IRQ_AUTODETECT | IRQ_WAITING);
		desc->handler->startup(irq);
	}
	spin_unlock_irqrestore(&desc->lock,flags);

	/* register_irq_proc(irq); */
	return 0;
}


/**
 *	request_irq - allocate an interrupt line
 *	@irq: Interrupt line to allocate
 *	@handler: Function to be called when the IRQ occurs
 *	@irqflags: Interrupt type flags
 *	@devname: An ascii name for the claiming device
 *	@dev_id: A cookie passed back to the handler function
 *
 *	This call allocates interrupt resources and enables the
 *	interrupt line and IRQ handling. From the point this
 *	call is made your handler function may be invoked. Since
 *	your handler function must clear any interrupt the board 
 *	raises, you must take care both to initialise your hardware
 *	and to set up the interrupt handler in the right order.
 *
 *	Dev_id must be globally unique. Normally the address of the
 *	device data structure is used as the cookie. Since the handler
 *	receives this value it makes sense to use it.
 *
 *	If your interrupt is shared you must pass a non NULL dev_id
 *	as this is required when freeing the interrupt.
 *
 *	Flags:
 *
 *	SA_SHIRQ		Interrupt is shared
 *
 *	SA_INTERRUPT		Disable local interrupts while processing
 *
 *	SA_SAMPLE_RANDOM	The interrupt can be used for entropy
 *
 */
 
int request_irq(unsigned int irq, 
		void (*handler)(int, void *, struct pt_regs *),
		unsigned long irqflags, 
		const char * devname,
		void *dev_id)
{
	int retval;
	struct irqaction * action;

#if 1
	/*
	 * Sanity-check: shared interrupts should REALLY pass in
	 * a real dev-ID, otherwise we'll have trouble later trying
	 * to figure out which interrupt is which (messes up the
	 * interrupt freeing logic etc).
	 */
	if (irqflags & SA_SHIRQ) {
		if (!dev_id)
			printk("Bad boy: %s (at 0x%x) called us without a dev_id!\n", devname, (&irq)[-1]);
	}
#endif

	if (irq >= NR_IRQS)
		return -EINVAL;
	if (!handler)
		return -EINVAL;

	action = (struct irqaction *)
			kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->next = NULL;
	action->dev_id = dev_id;

	retval = setup_irq(irq, action);
	if (retval)
		kfree(action);
	return retval;
}

/**
 *	free_irq - free an interrupt
 *	@irq: Interrupt line to free
 *	@dev_id: Device identity to free
 *
 *	Remove an interrupt handler. The handler is removed and if the
 *	interrupt line is no longer in use by any driver it is disabled.
 *	On a shared IRQ the caller must ensure the interrupt is disabled
 *	on the card it drives before calling this function. The function
 *	does not return until any executing interrupts for this IRQ
 *	have completed.
 *
 *	This function may be called from interrupt context. 
 *
 *	Bugs: Attempting to free an irq in a handler for the same irq hangs
 *	      the machine.
 */
 
void free_irq(unsigned int irq, void *dev_id)
{
	irq_desc_t *desc;
	struct irqaction **p;
	unsigned long flags;

	if (irq >= NR_IRQS)
		return;

	desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock,flags);
	p = &desc->action;
	for (;;) {
		struct irqaction * action = *p;
		if (action) {
			struct irqaction **pp = p;
			p = &action->next;
			if (action->dev_id != dev_id)
				continue;

			/* Found it - now remove it from the list of entries */
			*pp = action->next;
			if (!desc->action) {
				desc->status |= IRQ_DISABLED;
				desc->handler->shutdown(irq);
			}
			spin_unlock_irqrestore(&desc->lock,flags);

#ifdef CONFIG_SMP
			/* Wait to make sure it's not being used on another CPU */
			while (desc->status & IRQ_INPROGRESS)
				barrier();
#endif
			kfree(action);
			return;
		}
		printk("Trying to free free IRQ%d\n",irq);
		spin_unlock_irqrestore(&desc->lock,flags);
		return;
	}
}

/*
 * IRQ autodetection code..
 *
 * This depends on the fact that any interrupt that
 * comes in on to an unassigned handler will get stuck
 * with "IRQ_WAITING" cleared and the interrupt
 * disabled.
 */

static DECLARE_MUTEX(probe_sem);

/**
 *	probe_irq_on	- begin an interrupt autodetect
 *
 *	Commence probing for an interrupt. The interrupts are scanned
 *	and a mask of potential interrupt lines is returned.
 *
 */
 
unsigned long probe_irq_on(void)
{
	unsigned int i;
	irq_desc_t *desc;
	unsigned long val;
	unsigned long delay;

	down(&probe_sem);
	/* 
	 * something may have generated an irq long ago and we want to
	 * flush such a longstanding irq before considering it as spurious. 
	 */
	for (i = NR_IRQS-1; i > 0; i--)  {
		desc = irq_desc + i;

		spin_lock_irq(&desc->lock);
		if (!irq_desc[i].action) 
			irq_desc[i].handler->startup(i);
		spin_unlock_irq(&desc->lock);
	}

	/* Wait for longstanding interrupts to trigger. */
	for (delay = jiffies + HZ/50; time_after(delay, jiffies); )
		/* about 20ms delay */ synchronize_irq();

	/*
	 * enable any unassigned irqs
	 * (we must startup again here because if a longstanding irq
	 * happened in the previous stage, it may have masked itself)
	 */
	for (i = NR_IRQS-1; i > 0; i--) {
		desc = irq_desc + i;

		spin_lock_irq(&desc->lock);
		if (!desc->action) {
			desc->status |= IRQ_AUTODETECT | IRQ_WAITING;
			if (desc->handler->startup(i))
				desc->status |= IRQ_PENDING;
		}
		spin_unlock_irq(&desc->lock);
	}

	/*
	 * Wait for spurious interrupts to trigger
	 */
	for (delay = jiffies + HZ/10; time_after(delay, jiffies); )
		/* about 100ms delay */ synchronize_irq();

	/*
	 * Now filter out any obviously spurious interrupts
	 */
	val = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = irq_desc + i;
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if (status & IRQ_AUTODETECT) {
			/* It triggered already - consider it spurious. */
			if (!(status & IRQ_WAITING)) {
				desc->status = status & ~IRQ_AUTODETECT;
				desc->handler->shutdown(i);
			} else
				if (i < 32)
					val |= 1 << i;
		}
		spin_unlock_irq(&desc->lock);
	}

	return val;
}

/*
 * Return a mask of triggered interrupts (this
 * can handle only legacy ISA interrupts).
 */
 
/**
 *	probe_irq_mask - scan a bitmap of interrupt lines
 *	@val:	mask of interrupts to consider
 *
 *	Scan the ISA bus interrupt lines and return a bitmap of
 *	active interrupts. The interrupt probe logic state is then
 *	returned to its previous value.
 *
 *	Note: we need to scan all the irq's even though we will
 *	only return ISA irq numbers - just so that we reset them
 *	all to a known state.
 */
unsigned int probe_irq_mask(unsigned long val)
{
	int i;
	unsigned int mask;

	mask = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = irq_desc + i;
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if (status & IRQ_AUTODETECT) {
			if (i < 16 && !(status & IRQ_WAITING))
				mask |= 1 << i;

			desc->status = status & ~IRQ_AUTODETECT;
			desc->handler->shutdown(i);
		}
		spin_unlock_irq(&desc->lock);
	}
	up(&probe_sem);

	return mask & val;
}

/*
 * Return the one interrupt that triggered (this can
 * handle any interrupt source).
 */

/**
 *	probe_irq_off	- end an interrupt autodetect
 *	@val: mask of potential interrupts (unused)
 *
 *	Scans the unused interrupt lines and returns the line which
 *	appears to have triggered the interrupt. If no interrupt was
 *	found then zero is returned. If more than one interrupt is
 *	found then minus the first candidate is returned to indicate
 *	their is doubt.
 *
 *	The interrupt probe logic state is returned to its previous
 *	value.
 *
 *	BUGS: When used in a module (which arguably shouldnt happen)
 *	nothing prevents two IRQ probe callers from overlapping. The
 *	results of this are non-optimal.
 */
 
int probe_irq_off(unsigned long val)
{
	int i, irq_found, nr_irqs;

	nr_irqs = 0;
	irq_found = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = irq_desc + i;
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if (status & IRQ_AUTODETECT) {
			if (!(status & IRQ_WAITING)) {
				if (!nr_irqs)
					irq_found = i;
				nr_irqs++;
			}
			desc->status = status & ~IRQ_AUTODETECT;
			desc->handler->shutdown(i);
		}
		spin_unlock_irq(&desc->lock);
	}
	up(&probe_sem);

	if (nr_irqs > 1)
		irq_found = -irq_found;
	return irq_found;
}

/*
 * This should really return information about whether
 * we should do bottom half handling etc. Right now we
 * end up _always_ checking the bottom half, which is a
 * waste of time and is not what some drivers would
 * prefer.
 */
int handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqaction * action)
{
	int status;
	int cpu = smp_processor_id();

	irq_enter(cpu, irq);

	status = 1;	/* Force the "do bottom halves" bit */

	if (!(action->flags & SA_INTERRUPT))
		__sti();

	do {
		status |= action->flags;
		action->handler(irq, action->dev_id, regs);
		action = action->next;
	} while (action);
	if (status & SA_SAMPLE_RANDOM)
		add_interrupt_randomness(irq);
	__cli();

	irq_exit(cpu, irq);

	return status;
}

/*
 * Generic enable/disable code: this just calls
 * down into the PIC-specific version for the actual
 * hardware disable after having gotten the irq
 * controller lock. 
 */
 
/**
 *	disable_irq_nosync - disable an irq without waiting
 *	@irq: Interrupt to disable
 *
 *	Disable the selected interrupt line. Disables of an interrupt
 *	stack. Unlike disable_irq(), this function does not ensure existing
 *	instances of the IRQ handler have completed before returning.
 *
 *	This function may be called from IRQ context.
 */
 
void inline disable_irq_nosync(unsigned int irq)
{
	irq_desc_t *desc = irq_desc + irq;
	unsigned long flags;

	spin_lock_irqsave(&desc->lock, flags);
	if (!desc->depth++) {
		desc->status |= IRQ_DISABLED;
		desc->handler->disable(irq);
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}

/**
 *	disable_irq - disable an irq and wait for completion
 *	@irq: Interrupt to disable
 *
 *	Disable the selected interrupt line. Disables of an interrupt
 *	stack. That is for two disables you need two enables. This
 *	function waits for any pending IRQ handlers for this interrupt
 *	to complete before returning. If you use this function while
 *	holding a resource the IRQ handler may need you will deadlock.
 *
 *	This function may be called - with care - from IRQ context.
 */
 
void disable_irq(unsigned int irq)
{
	disable_irq_nosync(irq);

	if (!local_irq_count(smp_processor_id())) {
		do {
			barrier();
		} while (irq_desc[irq].status & IRQ_INPROGRESS);
	}
}

/**
 *	enable_irq - enable interrupt handling on an irq
 *	@irq: Interrupt to enable
 *
 *	Re-enables the processing of interrupts on this IRQ line
 *	providing no disable_irq calls are now in effect.
 *
 *	This function may be called from IRQ context.
 */
 
void enable_irq(unsigned int irq)
{
	irq_desc_t *desc = irq_desc + irq;
	unsigned long flags;

	spin_lock_irqsave(&desc->lock, flags);
	switch (desc->depth) {
	case 1: {
		unsigned int status = desc->status & ~IRQ_DISABLED;
		desc->status = status;
		if ((status & (IRQ_PENDING | IRQ_REPLAY)) == IRQ_PENDING) {
			desc->status = status | IRQ_REPLAY;
		}
		desc->handler->enable(irq);
		/* fall-through */
	}
	default:
		desc->depth--;
		break;
	case 0:
		printk("enable_irq(%u) unbalanced from %p\n", irq,
		       __builtin_return_address(0));
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}

/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */
asmlinkage unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	/* 
	 * We ack quickly, we don't want the irq controller
	 * thinking we're snobs just because some other CPU has
	 * disabled global interrupts (we have already done the
	 * INT_ACK cycles, it's too late to try to pretend to the
	 * controller that we aren't taking the interrupt).
	 *
	 * 0 return value means that this irq is already being
	 * handled by some other CPU. (or is disabled)
	 */
	int cpu = smp_processor_id();
	irq_desc_t *desc = irq_desc + irq;
	struct irqaction * action;
	unsigned int status;

	kstat.irqs[cpu][irq]++;
	spin_lock(&desc->lock);
	desc->handler->ack(irq);
	/*
	   REPLAY is when Linux resends an IRQ that was dropped earlier
	   WAITING is used by probe to mark irqs that are being tested
	   */
	status = desc->status & ~(IRQ_REPLAY | IRQ_WAITING);
	status |= IRQ_PENDING; /* we _want_ to handle it */

	/*
	 * If the IRQ is disabled for whatever reason, we cannot
	 * use the action we have.
	 */
	action = NULL;
	if (!(status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
		action = desc->action;
		status &= ~IRQ_PENDING; /* we commit to handling */
		status |= IRQ_INPROGRESS; /* we are handling it */
	}
	desc->status = status;

	/*
	 * If there is no IRQ handler or it was disabled, exit early.
	   Since we set PENDING, if another processor is handling
	   a different instance of this same irq, the other processor
	   will take care of it.
	 */
	if (!action)
		goto out;

	/*
	 * Edge triggered interrupts need to remember
	 * pending events.
	 * This applies to any hw interrupts that allow a second
	 * instance of the same irq to arrive while we are in do_IRQ
	 * or in the handler. But the code here only handles the _second_
	 * instance of the irq, not the third or fourth. So it is mostly
	 * useful for irq hardware that does not mask cleanly in an
	 * SMP environment.
	 */
	for (;;) {
		spin_unlock(&desc->lock);
		handle_IRQ_event(irq, regs, action);
		spin_lock(&desc->lock);
		
		if (!(desc->status & IRQ_PENDING))
			break;
		desc->status &= ~IRQ_PENDING;
	}
	desc->status &= ~IRQ_INPROGRESS;
out:
	/*
	 * The ->end() handler has to deal with interrupts which got
	 * disabled while the handler was running.
	 */
	desc->handler->end(irq);
	spin_unlock(&desc->lock);

	if (softirq_pending(cpu))
		do_softirq();
	return 1;
}
