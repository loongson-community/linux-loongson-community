/*
 * arch/mips/gt64120/common/irq.c
 *   Top-level irq code.  This is really common among all MIPS boards.
 *   Should be "upgraded" to arch/mips/kernel/irq.c
 *
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *   glonnon@ridgerun.com, skranz@ridgerun.com, stevej@ridgerun.com
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
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
 *
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
#include <linux/malloc.h>
#include <linux/random.h>
#include <linux/irq.h>
#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/pmc/ev64120int.h>


#undef IRQ_DEBUG

#ifdef IRQ_DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif


/*
 * Generic no controller code
 */

static void no_irq_enable_disable(unsigned int irq)
{
}

static unsigned int no_irq_startup(unsigned int irq)
{
	return 0;
}

static void no_irq_ack(unsigned int irq)
{
	printk(KERN_CRIT "Unexpected IRQ trap at vector %u\n", irq);
}

struct hw_interrupt_type no_irq_type = {
    typename:	"none",
    startup:	no_irq_startup,
    shutdown:	no_irq_enable_disable,
    enable:	no_irq_enable_disable,
    disable:	no_irq_enable_disable,
    // ack:        no_irq_ack, 	
    // [jsun] cannot use it yet. gt64120 does not have its own handler
    ack:	NULL,
    end:	no_irq_enable_disable,
};



/*
 * Controller mappings for all interrupt sources:
 */
irq_desc_t irq_desc[NR_IRQS];

unsigned long spurious_count = 0;

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

asmlinkage void do_IRQ(int irq, struct pt_regs *regs)
{
	struct irqaction *action;
	int cpu;
	cpu = smp_processor_id();
	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;
	if (irq_desc[irq].handler->ack) {
		irq_desc[irq].handler->ack(irq);
	}

	disable_irq(irq);

	action = irq_desc[irq].action;
	if (action && action->handler) {
#ifdef IRQ_DEBUG
		if (irq != TIMER)
			DBG(KERN_INFO
			    "rr: irq %d action %p and handler %p\n", irq,
			    action, action->handler);
#endif
		if (!(action->flags & SA_INTERRUPT))
			__sti();
		do {
			action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while (action);
		__cli();
		if (irq_desc[irq].handler) {
			if (irq_desc[irq].handler->end)
				irq_desc[irq].handler->end(irq);
			else if (irq_desc[irq].handler->enable)
				irq_desc[irq].handler->enable(irq);
		}
	}

	enable_irq(irq);
	irq_exit(cpu, irq);

	if (softirq_active(cpu) & softirq_mask(cpu))
		do_softirq();

	/* unmasking and bottom half handling is done magically for us. */
}

int request_irq(unsigned int irq,
		void (*handler) (int, void *, struct pt_regs *),
		unsigned long irqflags, const char *devname, void *dev_id)
{
	struct irqaction *old, **p, *action;
	unsigned long flags;
	if (irq >= NR_IRQS)
		return -EINVAL;

	action = (struct irqaction *)
	    kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = NULL;

	save_flags(flags);
	cli();

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

	restore_flags(flags);
	enable_irq(irq);

	return 0;
}


void free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction *p, *old = NULL;
	unsigned long flags;
	int count, tmp, removed = 0;

	for (p = irq_desc[irq].action; p != NULL; old = p, p = p->next) {
		/* Found the IRQ, is it the correct dev_id?  */
		if (dev_id == p->dev_id) {
			save_flags(flags);
			cli();

			// remove link from list
			if (old)
				old->next = p->next;
			else
				irq_desc[irq].action = p->next;

			restore_flags(flags);
			kfree(p);
			removed = 1;
			break;
		}
	}
}

unsigned long probe_irq_on(void)
{
	printk(KERN_INFO "probe_irq_on\n");
	return 0;
}

int probe_irq_off(unsigned long irqs)
{
	printk(KERN_INFO "probe_irq_off\n");
	return 0;
}

void __init init_IRQ(void)
{
	int i;

	DBG(KERN_INFO "rr:init_IRQ\n");

	/*  Let's initialize our IRQ descriptors  */
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status = 0;
		irq_desc[i].handler = &no_irq_type;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 0;
		irq_desc[i].lock = SPIN_LOCK_UNLOCKED;
	}

	irq_setup();
}

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-indent-level: 4 
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -4
 * c-argdecl-indent: 4
 * c-label-offset: -4
 * c-continued-statement-offset: 4
 * c-continued-brace-offset: 0
 * indent-tabs-mode: nil
 * tab-width: 8
 * End:
 */
