/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Routines for generic manipulation of the interrupts found on the MIPS 
 * Atlas board.
 *
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>

#include <asm/irq.h>
#include <asm/mips-boards/atlas.h>
#include <asm/mips-boards/atlasint.h>


struct atlas_ictrl_regs *atlas_hw0_icregs;

extern asmlinkage void mipsIRQ(void);

unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];
unsigned long spurious_count = 0;

static struct irqaction *hw0_irq_action[ATLASINT_END] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

static struct irqaction r4ktimer_action = {
	NULL, 0, 0, "R4000 timer/counter", NULL, NULL,
};

static struct irqaction *irq_action[8] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, &r4ktimer_action
};

#if 0
#define DEBUG_INT(x...) printk(x)
#else
#define DEBUG_INT(x...)
#endif

void disable_irq(unsigned int irq_nr)
{
        unsigned long flags;

	if(irq_nr >= ATLASINT_END) {
		printk("whee, invalid irq_nr %d\n", irq_nr);
		panic("IRQ, you lose...");
	}

	save_and_cli(flags);
	atlas_hw0_icregs->intrsten = (1 << irq_nr);
	restore_flags(flags);
}


void enable_irq(unsigned int irq_nr)
{
        unsigned long flags;

	if(irq_nr >= ATLASINT_END) {
		printk("whee, invalid irq_nr %d\n", irq_nr);
		panic("IRQ, you lose...");
	}

	save_and_cli(flags);
	atlas_hw0_icregs->intseten = (1 << irq_nr);
	restore_flags(flags);
}


int get_irq_list(char *buf)
{
	int i, len = 0;
	int num = 0;
	struct irqaction *action;

	for (i = 0; i < 8; i++, num++) {
		action = irq_action[i];
		if (!action) 
			continue;
		len += sprintf(buf+len, "%2d: %8d %c %s",
			num, kstat.irqs[0][num],
			(action->flags & SA_INTERRUPT) ? '+' : ' ',
			action->name);
		for (action=action->next; action; action = action->next) {
			len += sprintf(buf+len, ",%s %s",
				(action->flags & SA_INTERRUPT) ? " +" : "",
				action->name);
		}
		len += sprintf(buf+len, " [on-chip]\n");
	}
	for (i = 0; i < ATLASINT_END; i++, num++) {
		action = hw0_irq_action[i];
		if (!action) 
			continue;
		len += sprintf(buf+len, "%2d: %8d %c %s",
			num, kstat.irqs[0][num],
			(action->flags & SA_INTERRUPT) ? '+' : ' ',
			action->name);
		for (action=action->next; action; action = action->next) {
			len += sprintf(buf+len, ",%s %s",
				(action->flags & SA_INTERRUPT) ? " +" : "",
				action->name);
		}
		len += sprintf(buf+len, " [hw0]\n");
	}
	return len;
}


int request_irq(unsigned int irq, 
		void (*handler)(int, void *, struct pt_regs *),
		unsigned long irqflags, 
		const char * devname,
		void *dev_id)
{  
        struct irqaction *action;

	DEBUG_INT("request_irq: irq=%d, devname = %s\n", irq, devname);

        if (irq >= ATLASINT_END)
	        return -EINVAL;
	if (!handler)
	        return -EINVAL;

	action = (struct irqaction *)kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if(!action)
	        return -ENOMEM;

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = 0;
	hw0_irq_action[irq] = action;
	enable_irq(irq);

	return 0;
}


void free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction *action;

	if (irq >= ATLASINT_END) {
		printk("Trying to free IRQ%d\n",irq);
		return;
	}

	action = hw0_irq_action[irq];
	hw0_irq_action[irq] = NULL;
	disable_irq(irq);
	kfree(action);
}

void __init init_IRQ(void)
{
	irq_setup();
}

void atlas_hw0_irqdispatch(struct pt_regs *regs)
{
        struct irqaction *action;
	int irq, cpu = smp_processor_id();
	unsigned long int_status;
	int i;

	DEBUG_INT("atlas_hw0_irqdispatch\n");
	
	int_status = atlas_hw0_icregs->intstatus; 

	/* if int_status == 0, then the interrupt has already been cleared */
	if (int_status == 0)
		return;

	for (i=0; i<ATLASINT_END; i++)
		if (int_status & 1<<i)
			break;
	
	irq = i;
	action = hw0_irq_action[irq];

	DEBUG_INT("atlas_hw0_irqdispatch: irq=%d\n", irq);

	/* if action == NULL, then we don't have a handler for the irq */
	if (action == NULL) {
	        printk("No handler for hw0 irq: %i\n", irq);
		return;
	}

	irq_enter(cpu);
	kstat.irqs[0][irq + 8]++;
	action->handler(irq, action->dev_id, regs);
	irq_exit(cpu);

	return;		
}


/* Misc. crap just to keep the kernel linking... */
unsigned long probe_irq_on (void)
{
	return 0;
}


int probe_irq_off (unsigned long irqs)
{
	return 0;
}


void __init atlasint_init(void)
{
        /* 
	 * Mask out all interrupt by writing "1" to all bit position in 
	 * the interrupt reset reg. 
	 */
        atlas_hw0_icregs = (struct atlas_ictrl_regs *)ATLAS_ICTRL_REGS_BASE;
	atlas_hw0_icregs->intrsten = 0xffffffff;    
	
	/* Now safe to set the exception vector. */
	set_except_vector(0, mipsIRQ);
}
