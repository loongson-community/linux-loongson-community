/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Galileo EV96100 interrupt/setup routines.
 *
 * Copyright 2000 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or support@mvista.com
 *
 * This file was derived from Carsten Langgaard's 
 * arch/mips/mips-boards/atlas/atlas_int.c.
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/galileo-boards/ev96100int.h>

extern void mips_timer_interrupt(int irq, struct pt_regs *regs);
extern asmlinkage void ev96100IRQ(void);
irq_cpustat_t irq_stat[NR_CPUS];
unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];
unsigned long spurious_count = 0;
irq_desc_t irq_desc[NR_IRQS];
irq_desc_t *irq_desc_base = &irq_desc[0];

static struct irqaction timer_action = {
	NULL, 0, 0, "R7000 timer/counter", NULL, NULL,
};

static struct hw_interrupt_type mips_timer = {
	"MIPS CPU Timer",
	NULL,
	NULL,
	NULL,			/* unmask_irq */
	NULL,			/* mask_irq */
	NULL,			/* mask_and_ack */
	0
};

/* Function for careful CP0 interrupt mask access */
static inline void modify_cp0_intmask(unsigned clr_mask, unsigned set_mask)
{
	unsigned long status = read_32bit_cp0_register(CP0_STATUS);
	status &= ~((clr_mask & 0xFF) << 8);
	status |= (set_mask & 0xFF) << 8;
	write_32bit_cp0_register(CP0_STATUS, status);
}

static inline void mask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(irq_nr, 0);
}

static inline void unmask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(0, irq_nr);
}

void disable_irq(unsigned int irq_nr)
{
	unsigned long flags;

	save_and_cli(flags);
	mask_irq(irq_nr);
	restore_flags(flags);
}

void enable_irq(unsigned int irq_nr)
{
	unsigned long flags;

#if 0
	printk("enable irq %d\n", irq_nr);
	printk("status reg: %x, cause %x\n",
	       read_32bit_cp0_register(CP0_STATUS),
	       read_32bit_cp0_register(CP0_CAUSE));
#endif
	save_and_cli(flags);
	unmask_irq(irq_nr);
	restore_flags(flags);
#if 0
	printk("new status reg: %x, cause %x\n",
	       read_32bit_cp0_register(CP0_STATUS),
	       read_32bit_cp0_register(CP0_CAUSE));
#endif
}


void __init ev96100_time_init()
{
	puts("ev96100 time_init\n");

}

int get_irq_list(char *buf)
{
	int i, len = 0, j;
	struct irqaction *action;

	len += sprintf(buf + len, "           ");
	for (j = 0; j < smp_num_cpus; j++)
		len += sprintf(buf + len, "CPU%d       ", j);
	*(char *) (buf + len++) = '\n';

	for (i = 0; i < NR_IRQS; i++) {
		action = irq_desc[i].action;
		if (!action || !action->handler)
			continue;
		len += sprintf(buf + len, "%3d: ", i);
		len += sprintf(buf + len, "%10u ", kstat_irqs(i));
		if (irq_desc[i].handler)
			len +=
			    sprintf(buf + len, " %s ",
				    irq_desc[i].handler->typename);
		else
			len += sprintf(buf + len, "  None      ");
		len += sprintf(buf + len, "    %s", action->name);
		for (action = action->next; action; action = action->next) {
			len += sprintf(buf + len, ", %s", action->name);
		}
		len += sprintf(buf + len, "\n");
	}
	len += sprintf(buf + len, "BAD: %10lu\n", spurious_count);
	return len;
}

asmlinkage void do_IRQ(unsigned long cause, struct pt_regs *regs)
{
	struct irqaction *action;
	int cpu;
	int status;
	int irq;

	//printk("do_IRQ: cause %x *regs %x\n", cause, regs);
	/*
	 * Service one interrupt only.
	 * The "priority" is not really defined at this point.
	 * This will change once we add the R7000 extensions.
	 */
//        if (cause & CAUSEF_IP7)
//            irq = 7;
	if (cause & CAUSEF_IP6)
		irq = 6;
	else if (cause & CAUSEF_IP5)
		irq = 5;
	else if (cause & CAUSEF_IP4)
		irq = 4;
	else if (cause & CAUSEF_IP3)
		irq = 3;
	else if (cause & CAUSEF_IP2)
		irq = 2;
	else if (cause & CAUSEF_IP1)
		irq = 1;
	else if (cause & CAUSEF_IP0)
		irq = 0;
	else
		return;		/* should not happen */

	cpu = smp_processor_id();
	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;
	status = 0;

	if (irq_desc[irq].handler && irq_desc[irq].handler->ack) {
		irq_desc[irq].handler->ack(irq);
	}

	action = irq_desc[irq].action;

	if (action && action->handler) {
		//if (!(action->flags & SA_INTERRUPT)) __sti();
		//printk("irq %d, action->handler %x\n", irq, action->handler);
		do {
			status |= action->flags;
			action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while (action);
		//__cli();
		if (irq_desc[irq].handler) {
			printk("handler??\n");
			while (1);
			if (irq_desc[irq].handler->end)
				irq_desc[irq].handler->end(irq);
			else if (irq_desc[irq].handler->enable)
				irq_desc[irq].handler->enable(irq);
		}
	} else {
		spurious_count++;
		//printk(KERN_DEBUG "Unhandled interrupt %x, disabled\n", irq);
		printk("Unhandled interrupt %x, disabled\n", irq);
		disable_irq(1 << irq);
		if (irq_desc[irq].handler->end)
			irq_desc[irq].handler->end(irq);
	}

	irq_exit(cpu, irq);

	if (softirq_active(cpu) & softirq_mask(cpu))
		do_softirq();
}

int request_irq(unsigned int irq,
		void (*handler) (int, void *, struct pt_regs *),
		unsigned long irqflags, const char *devname, void *dev_id)
{
	struct irqaction *old, **p, *action;
	unsigned long flags;

	/*
	 * IRQs are number 0 through 7, where 0 corresponds to IP0 and
	 * 7 corresponds to IP7.  IP0 and IP1 are software interrupts. IP7
	 * is typically the timer interrupt, unless the R7000 extensions are
	 * used.
	 */

	printk("request_irq %d, handler %x\n", irq, handler);

	if (irq >= NR_IRQS)
		return -EINVAL;
	if (!handler) {
		/* Free */
		for (p = &irq_desc[irq].action; (action = *p) != NULL;
		     p = &action->next) {
			/* Found it - now free it */
			save_flags(flags);
			cli();
			*p = action->next;
			restore_flags(flags);
			kfree(action);
			return 0;
		}
		return -ENOENT;
	}

	action = (struct irqaction *)
	    kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;
	memset(action, 0, sizeof(struct irqaction));

	save_flags(flags);
	cli();

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = NULL;

	p = &irq_desc[irq].action;

	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & action->flags & SA_SHIRQ))
			return -EBUSY;
		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
	}
	*p = action;
	printk("action %x, action->handler %x\n",
	       irq_desc[irq].action, irq_desc[irq].action->handler);

	enable_irq(1 << irq);
	restore_flags(flags);
	return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
	printk("free_irq %d\n", irq);
	request_irq(irq, NULL, 0, NULL, dev_id);
}


unsigned long probe_irq_on(void)
{
	return 0;
}

int probe_irq_off(unsigned long irqs)
{
	return 0;
}

int (*irq_cannonicalize) (int irq);

int ev96100_irq_cannonicalize(int i)
{
	return i;
}

void __init init_IRQ(void)
{
	puts("init_IRQ\n");
	memset(irq_desc, 0, sizeof(irq_desc));
	irq_cannonicalize = ev96100_irq_cannonicalize;

	/*
	   irq_desc[EV96100INT_TIMER].handler = &mips_timer;
	   irq_desc[EV96100INT_TIMER].action = &timer_action;
	   irq_desc[EV96100INT_TIMER].action->handler = mips_timer_interrupt;
	 */

	set_except_vector(0, ev96100IRQ);
}

EXPORT_SYMBOL(irq_cannonicalize);
