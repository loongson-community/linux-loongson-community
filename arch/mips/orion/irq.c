/*
 * Code to handle irqs on Orion boards
 *  -- Cort <cort@fsmlabs.com>
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

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/orion.h>

void (*board_time_init)(struct irqaction *irq);
extern asmlinkage void orionIRQ(void);
irq_cpustat_t irq_stat [NR_CPUS];
unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];
unsigned long spurious_count = 0;
irq_desc_t irq_desc[NR_IRQS];
irq_desc_t *irq_desc_base=&irq_desc[0];

static void galileo_ack(unsigned int irq_nr)
{
	*((unsigned long *) (((unsigned)(0x14000000)|0xA0000000) + 0xC18)) =
		cpu_to_le32(0);
}

struct hw_interrupt_type galileo_pic = {
        " Galileo  ",
        NULL,
        NULL,
        NULL, /* unmask_irq */
        NULL, /* mask_irq */
        galileo_ack, /* mask_and_ack */
        0
};

/* Function for careful CP0 interrupt mask access */
static inline void modify_cp0_intmask(unsigned clr_mask, unsigned set_mask)
{
	unsigned long status = read_32bit_cp0_register(CP0_STATUS);
	status &= ~((clr_mask & 0xFF) << 8);
	status |=   (set_mask & 0xFF) << 8;
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

	save_and_cli(flags);
	unmask_irq(irq_nr);
	restore_flags(flags);
}

/*static struct irqaction *irq_action[NR_IRQS] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
*/

void __init orion_time_init(struct irqaction *irq)
{
	irq_desc[2].handler = &galileo_pic;
	irq_desc[2].action = irq;
	
	/* This code was provided by the CoSine guys and despite its
	 * appearance init's the timer.
	 * -- Cort
	 */
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x864)) =
		cpu_to_le32(0);
	
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x850)) =
		cpu_to_le32(0);

	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x850)) =
		cpu_to_le32(50000000/HZ);

	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0xC1C)) =
		cpu_to_le32(0x100);
     
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0x864)) =
		cpu_to_le32(0x03);
     
	*((__u32 *) (((unsigned)(0x14000000)|0xA0000000) + 0xC18)) =
		cpu_to_le32(0);
}

int get_irq_list(char *buf)
{
	int i, len = 0, j;
	struct irqaction * action;

	len += sprintf(buf+len, "           ");
	for (j=0; j<smp_num_cpus; j++)
		len += sprintf(buf+len, "CPU%d       ",j);
	*(char *)(buf+len++) = '\n';

	for (i = 0 ; i < NR_IRQS ; i++) {
		action = irq_desc[i].action;
		if ( !action || !action->handler )
			continue;
		len += sprintf(buf+len, "%3d: ", i);		
		len += sprintf(buf+len, "%10u ", kstat_irqs(i));
		if ( irq_desc[i].handler )		
			len += sprintf(buf+len, " %s ", irq_desc[i].handler->typename );
		else
			len += sprintf(buf+len, "  None      ");
		len += sprintf(buf+len, "    %s",action->name);
		for (action=action->next; action; action = action->next) {
			len += sprintf(buf+len, ", %s", action->name);
		}
		len += sprintf(buf+len, "\n");
	}
	len += sprintf(buf+len, "BAD: %10lu\n", spurious_count);
	return len;
}

asmlinkage void do_IRQ(int irq, struct pt_regs * regs)
{
	struct irqaction *action;
	int cpu;
	int status;

	cpu = smp_processor_id();
	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;
	status = 0;

	if (irq_desc[irq].handler->ack)
		irq_desc[irq].handler->ack(irq);
	
	action = irq_desc[irq].action;
	if (action && action->handler)
	{
		if (!(action->flags & SA_INTERRUPT))
			__sti();
		do { 
			status |= action->flags;
			action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while ( action );
		__cli();
		if (irq_desc[irq].handler)
		{
			if (irq_desc[irq].handler->end)
				irq_desc[irq].handler->end(irq);
			else if (irq_desc[irq].handler->enable)
				irq_desc[irq].handler->enable(irq);
		}
	}
	else
	{
		spurious_count++;
		printk(KERN_DEBUG "Unhandled interrupt %x, disabled\n", irq);
		disable_irq(irq);
		if (irq_desc[irq].handler->end)
			irq_desc[irq].handler->end(irq);
	}

	irq_exit(cpu, irq);

	if (softirq_active(cpu) & softirq_mask(cpu))
		do_softirq();
}

int request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
	unsigned long irqflags, const char * devname, void *dev_id)
{
	struct irqaction *old, **p, *action;
	unsigned long flags;

	if (irq >= NR_IRQS)
		return -EINVAL;
	if (!handler)
	{
		/* Free */
		for (p = &irq_desc[irq].action; (action = *p) != NULL; p = &action->next)
		{
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
	
	save_flags(flags);
	cli();
	
	action->handler = handler;
	action->flags = irqflags;					
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = NULL;
	enable_irq(irq);
	
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
	return 0;
}
		
void free_irq(unsigned int irq, void *dev_id)
{
	request_irq(irq, NULL, 0, NULL, dev_id);
}


unsigned long probe_irq_on (void)
{
	return 0;
}

int probe_irq_off (unsigned long irqs)
{
	return 0;
}

void __init init_IRQ(void)
{
	set_except_vector(0, orionIRQ);
}
