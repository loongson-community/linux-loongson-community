/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/linkage.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/irq_cpustat.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/signal.h>
#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/sibyte/64bit.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/sibyte/sb1250_scd.h>
#include <asm/sibyte/sb1250.h>

/* Sanity check.  We're an sb1250, with 2 SB1 cores on die; we'd better have configured for 
   an sb1 cpu */
#ifndef CONFIG_CPU_SB1
#error "Sb1250 requires configuration of SB1 cpu"
#endif

/*
 *  These are the routines that handle all the low level interrupt stuff. 
 *  Actions handled here are: initialization of the interrupt map, 
 *  requesting of interrupt lines by handlers, dispatching if interrupts
 *   to handlers, probing for interrupt lines */


static spinlock_t irq_mask_lock = SPIN_LOCK_UNLOCKED;


#define IMR_MASK(cpu)     (*(volatile unsigned long *)(IO_SPACE_BASE | A_IMR_REGISTER(cpu, R_IMR_INTERRUPT_MASK)))
#define IMR_REG(cpu, ofs) (*(volatile unsigned long *)((IO_SPACE_BASE | A_IMR_REGISTER(cpu, R_IMR_INTERRUPT_MAP_BASE)) + (ofs<<3)))
#define IMR_MAILBOX(cpu)  (*(volatile unsigned long *)(IO_SPACE_BASE | A_IMR_REGISTER(cpu, R_IMR_MAILBOX_CPU)))



/*
 * These functions (sb1250_mask_irq and sb1250_unmask_irq) provide
 * a safe, locked way to enable and disable interrupts at the
 * hardware level.  Note they only operate on cpu0; all irq's
 * are routed to cpu0 except the mailbox interrupt needed
 * for inter-processor-interrupts, so these are usable for
 * any other kind of irq.  
 */
void sb1250_mask_irq(int cpu, int irq)
{
	unsigned long flags;
	u64 cur_ints;
	spin_lock_irqsave(&irq_mask_lock, flags);
	cur_ints = in64(KSEG1 + A_IMR_MAPPER(cpu) + R_IMR_INTERRUPT_MASK);
	cur_ints |= (((u64)1)<<irq);
	out64(cur_ints, KSEG1 + A_IMR_MAPPER(cpu) + R_IMR_INTERRUPT_MASK);
	spin_unlock_irqrestore(&irq_mask_lock, flags);
}

void sb1250_unmask_irq(int cpu, int irq)
{
	unsigned long flags;
	u64 cur_ints;
	spin_lock_irqsave(&irq_mask_lock, flags);
	cur_ints = in64(KSEG1 + A_IMR_MAPPER(cpu) + R_IMR_INTERRUPT_MASK);
	cur_ints &= ~(((u64)1)<<irq);
	out64(cur_ints, KSEG1 + A_IMR_MAPPER(cpu) + R_IMR_INTERRUPT_MASK);
	spin_unlock_irqrestore(&irq_mask_lock, flags);
}


static unsigned int startup_none(unsigned int irq)
{
	/* Do nothing */
	return 0;
}

static void shutdown_none(unsigned int irq)
{
	/* Do nothing */
}

#define enable_none shutdown_none
#define disable_none shutdown_none
#define ack_none shutdown_none
#define end_none shutdown_none

static void affinity_none(unsigned int irq, unsigned long mask)
{
	/* Do nothing */
}


/*
 *  If depth is 0, then unmask the interrupt.  Increment depth 
 */
void enable_irq(unsigned int irq)
{
	
	unsigned long flags;
	irq_desc_t *desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock, flags);
	if (!desc->depth) {
		u64 cur_ints;
		spin_lock(&irq_mask_lock);
		cur_ints = in64(KSEG1 + A_IMR_MAPPER(0) + R_IMR_INTERRUPT_MASK);
		cur_ints &= ~(((u64)1)<<irq);
		out64(cur_ints, KSEG1 + A_IMR_MAPPER(0) + R_IMR_INTERRUPT_MASK);
		spin_unlock(&irq_mask_lock);
		desc->status &= ~IRQ_DISABLED;
	}
	desc->depth++;
	spin_unlock_irqrestore(&desc->lock, flags);
}

/*
 * If depth hits 0, disable the interrupt.  By grabbing desc->lock, we
 * implicitly sync because a dispatched interrupt will grab that lock
 * before calling handlers.  IRQ_DISABLED is checked by the dispatcher
 * under desc->lock, so we set that in case we're racing with a dispatch.
 */
void disable_irq(unsigned int irq)
{
	unsigned long flags;
	irq_desc_t *desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock, flags);
	if (!desc->depth) {
		BUG();
	}
	desc->depth--;
	if (!desc->depth) {
		u64 cur_ints;
		spin_lock(&irq_mask_lock);
		cur_ints = in64(KSEG1 + A_IMR_MAPPER(0) + R_IMR_INTERRUPT_MASK);
		cur_ints |= (((u64)1)<<irq);
		out64(cur_ints, KSEG1 + A_IMR_MAPPER(0) + R_IMR_INTERRUPT_MASK);
		spin_unlock(&irq_mask_lock);
		desc->status |= IRQ_DISABLED;
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}







static struct hw_interrupt_type no_irq_type = {
	"none",
	startup_none,
	shutdown_none,
	enable_none,
	disable_none,
	ack_none,
	end_none,
	affinity_none
};

/* 
 * irq_desc is the structure that keeps track of the state
 * and handlers for each of the IRQ lines
 */
static irq_desc_t irq_desc[NR_IRQS] =
{ [0 ... NR_IRQS-1] = { 0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};


/* Defined in arch/mips/sibyte/sb1250/irq_handler.S */
extern void sb1250_irq_handler(void);
/* 
 *  irq_err_count is used in arch/mips/kernel/entry.S to record the 
 *  number of spurious interrupts we see before the handler is installed. 
 *  It doesn't provide any particularly relevant information for us, so
 *  we basically ignore it.
 */ 
volatile unsigned long irq_err_count;

/*
 * The interrupt handler calls this once for every unmasked interrupt
 * that is pending.  vector is the IRQ number that was raised 
 */

void sb1250_dispatch_irq(unsigned int vector, struct pt_regs *regs)
{
	struct irqaction *action;
	irq_desc_t *desc = irq_desc + vector;
	/* XXX This race nipping stuff seems to be incorrect.  Revisit this later. */
	/*spin_lock(&desc->lock);*/
	/* Nip a race.  If we had a disable_irq() call that happened after we
	   started the interrupt but before we got here, it set the DISABLED 
	   flag.  Check that flag under the lock before doing anything */
	if (!(desc->status & IRQ_DISABLED)) {
		for (action = desc->action; action != NULL; action = action->next) {
			if (action->handler != NULL) {
				(*(action->handler))(vector, action->dev_id, regs);
			} 
		}
	}
	/*spin_unlock(&desc->lock);*/
}

/* 
 * Stolen, pretty much intact, from arch/i386/kernel/irq.c
 */

int setup_irq(unsigned int irq, struct irqaction * new)
{
	int shared = 0;
	unsigned long flags;
	struct irqaction *old, **p;
	irq_desc_t *desc = irq_desc + irq;

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
	return 0;
}


/*
 *  init_IRQ is called early in the boot sequence from init/main.c.  It
 *  is responsible for setting up the interrupt mapper and installing the
 *  handler that will be responsible for dispatching interrupts to the
 *  "right" place. 
 */
/*
 * For now, map all interrupts to IP[2].  We could save
 * some cycles by parceling out system interrupts to different
 * IP lines, but keep it simple for bringup.  We'll also direct
 * all interrupts to a single CPU; we should probably route
 * PCI and LDT to one cpu and everything else to the other
 * to balance the load a bit. 
 * 
 * On the second cpu, everything is set to IP5, which is
 * ignored, EXCEPT the mailbox interrupt.  That one is 
 * set to IP2 so it is handled.  This is needed so we
 * can do cross-cpu function calls, as requred by SMP
 */
void __init init_IRQ(void)
{

	unsigned int i;
	u64 tmp;
	/* Default everything to IP2 */
	for (i = 0; i < 64; i++) {
		IMR_REG(0, i) = K_INT_MAP_I0;
		IMR_REG(1, i) = K_INT_MAP_I0;
	}

	/* Map general purpose timer 0 to IP[4] on cpu 0.  This is the system timer */
	IMR_REG(0, K_INT_TIMER_0) = K_INT_MAP_I2;

	/* Map the high 16 bits of the mailbox registers to IP[3], for inter-cpu messages */
	IMR_REG(0, K_INT_MBOX_0) = K_INT_MAP_I1;
	IMR_REG(1, K_INT_MBOX_0) = K_INT_MAP_I1;

	/* Clear the mailboxes.  The firmware may leave them dirty */
	IMR_MAILBOX(0) = 0;
	IMR_MAILBOX(1) = 0;

	/* Mask everything except the mailbox registers for both cpus */
	tmp = ~((u64)0) ^ (((u64)1) << K_INT_MBOX_0);
	IMR_MASK(0) = tmp;
	IMR_MASK(1) = tmp;
	
	/* Enable IP[4:0], disable the rest */
	set_cp0_status(0xff00, 0x1f00);

	set_except_vector(0, sb1250_irq_handler);
}


/*
 *  request_irq() is called by drivers to request addition to the chain
 *  of handlers called for a given interrupt.  
 *
 * arch/i386/kernel/irq.c says this is going to become generic code in 2.5
 * Makes sense, considering it's already architecture independent.  As such, 
 * I've tried to match the i386 style as much as possible to make the 
 * transition simple
 */
int request_irq(unsigned int irq, 
		void (*handler)(int, void *, struct pt_regs *),
		unsigned long irqflags, const char * devname, void *dev_id)
{
	int retval;
	struct irqaction * action;
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

	if (irq >= NR_IRQS) {
		return -EINVAL;
	}
	if (!handler) {
		return -EINVAL;
	}

	action = (struct irqaction *) kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action) {
		return -ENOMEM;
	}

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->next = NULL;
	action->dev_id = dev_id;

	retval = setup_irq(irq, action);
	if (retval) {
		kfree(action);
	}
	else {
	    if ((irq >= 56) && (irq <= 59)) {
		sb1250_unmask_irq(0,irq);
		}
	}
	return retval;
}


/*
 *  free_irq() releases a handler set up by request_irq()
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
		spin_unlock_irqrestore(&desc->lock,flags);
		return;
	}
}

/*
 *  get_irq_list() pretty prints a list of who has requested which
 *  irqs.  This function is activated by a read of a file in /proc/
 *  Returns the length of the string generated
 *
 */
int get_irq_list(char *buf)
{
	return 0;
}

/*
 *  probe_irq_on() and probe_irq_off() are a pair of functions used for 
 *  determining which interrupt a device is using.  I *think* this is
 *  pretty much legacy for [E]ISA applications, thus the stubbing out.
 */

unsigned long probe_irq_on (void)
{ 
	panic("probe_irq_on called");
}

int probe_irq_off (unsigned long irqs)
{
	panic("probe_irq_off called");
}


/*
 * I don't see a good way to do this without syncing, given the
 * way desc->lock works, so just sync anyways.
 */
void disable_irq_nosync(unsigned int irq)
{
	disable_irq(irq);
}

