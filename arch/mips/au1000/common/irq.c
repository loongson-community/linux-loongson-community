/*
 * BRIEF MODULE DESCRIPTION
 *	Au1000 interrupt routines.
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *		ppopov@mvista.com or source@mvista.com
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED	  ``AS	IS'' AND   ANY	EXPRESS OR IMPLIED
 *  WARRANTIES,	  INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO	EVENT  SHALL   THE AUTHOR  BE	 LIABLE FOR ANY	  DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED	  TO, PROCUREMENT OF  SUBSTITUTE GOODS	OR SERVICES; LOSS OF
 *  USE, DATA,	OR PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN	 CONTRACT, STRICT LIABILITY, OR TORT
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
#include <linux/malloc.h>
#include <linux/random.h>

#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/au1000.h>

#define ALLINTS (IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4 | IE_IRQ5)

#undef DEBUG_IRQ
#ifdef DEBUG_IRQ
/* note: prints function name for you */
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define EXT_INTC0_REQ0 2 /* IP 2 */
#define EXT_INTC0_REQ1 3 /* IP 3 */
#define EXT_INTC1_REQ0 4 /* IP 4 */
#define EXT_INTC1_REQ1 5 /* IP 5 */
#define MIPS_TIMER_IP  7 /* IP 7 */

#ifdef CONFIG_REMOTE_DEBUG
extern void breakpoint(void);
#endif

extern asmlinkage void au1000_IRQ(void);
extern void set_debug_traps(void);
irq_cpustat_t irq_stat [NR_CPUS];
unsigned int local_bh_count[NR_CPUS];
unsigned int local_irq_count[NR_CPUS];
unsigned long spurious_count = 0;
irq_desc_t irq_desc[NR_IRQS];

static void setup_au1000_irq(unsigned int irq, int type, int int_req);
static unsigned int startup_au1000_irq(unsigned int irq);
static void enable_au1000_irq(unsigned int irq_nr);
static void disable_au1000_irq(unsigned int irq_nr);
static void end_au1000_irq(unsigned int irq_nr);

static void ack_level_irq(unsigned int irq_nr);
static void ack_rise_edge_irq(unsigned int irq_nr);
static void ack_fall_edge_irq(unsigned int irq_nr);

void disable_ack_irq(int irq);

/* Function for careful CP0 interrupt mask access */
static inline void modify_cp0_intmask(unsigned clr_mask, unsigned set_mask)
{
	unsigned long status = read_32bit_cp0_register(CP0_STATUS);
	status &= ~((clr_mask & 0xFF) << 8);
	status |=   (set_mask & 0xFF) << 8;
	write_32bit_cp0_register(CP0_STATUS, status);
}

static inline void mask_cpu_irq_input(unsigned int irq_nr)
{
	modify_cp0_intmask(irq_nr, 0);
}

static inline void unmask_cpu_irq_input(unsigned int irq_nr)
{
	modify_cp0_intmask(0, irq_nr);
}

static void disable_cpu_irq_input(unsigned int irq_nr)
{
	unsigned long flags;

	save_and_cli(flags);
	mask_cpu_irq_input(irq_nr);
	restore_flags(flags);
}

static void enable_cpu_irq_input(unsigned int irq_nr)
{
	unsigned long flags;

	save_and_cli(flags);
	unmask_cpu_irq_input(irq_nr);
	restore_flags(flags);
}


static void setup_au1000_irq(unsigned int irq_nr, int type, int int_req)
{
	/* Config2[n], Config1[n], Config0[n] */
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		switch (type) {
			case INTC_INT_RISE_EDGE: /* 0:0:1 */
				outl(1<<irq_nr,INTC1_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG0_SET);
				break;
			case INTC_INT_FALL_EDGE: /* 0:1:0 */
				outl(1<<irq_nr, INTC1_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG1_SET);
				outl(1<<irq_nr, INTC1_CONFIG0_CLEAR);
				break;
			case INTC_INT_HIGH_LEVEL: /* 1:0:1 */
				outl(1<<irq_nr, INTC1_CONFIG2_SET);
				outl(1<<irq_nr, INTC1_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG0_SET);
				break;
			case INTC_INT_LOW_LEVEL: /* 1:1:0 */
				outl(1<<irq_nr, INTC1_CONFIG2_SET);
				outl(1<<irq_nr, INTC1_CONFIG1_SET);
				outl(1<<irq_nr, INTC1_CONFIG0_CLEAR);
				break;
			case INTC_INT_DISABLED: /* 0:0:0 */
				outl(1<<irq_nr, INTC1_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG2_CLEAR);
				break;
			default: /* disable the interrupt */
				printk("unexpected int type %d (irq %d)\n", type, irq_nr);
				outl(1<<irq_nr, INTC1_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC1_CONFIG2_CLEAR);
				return;
		}
		if (int_req) /* assign to interrupt request 1 */
			outl(1<<irq_nr, INTC1_ASSIGN_REQ_CLEAR);
		else	     /* assign to interrupt request 0 */
			outl(1<<irq_nr, INTC1_ASSIGN_REQ_SET);
		outl(1<<irq_nr, INTC1_SOURCE_SET);
		outl(1<<irq_nr, INTC1_MASK_CLEAR);
	}
	else {
		switch (type) {
			case INTC_INT_RISE_EDGE: /* 0:0:1 */
				printk("irq %d: rise edge\n", irq_nr);
				outl(1<<irq_nr,INTC0_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG0_SET);
				break;
			case INTC_INT_FALL_EDGE: /* 0:1:0 */
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_SET);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				break;
			case INTC_INT_HIGH_LEVEL: /* 1:0:1 */
				outl(1<<irq_nr, INTC0_CONFIG2_SET);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG0_SET);
				break;
			case INTC_INT_LOW_LEVEL: /* 1:1:0 */
				outl(1<<irq_nr, INTC0_CONFIG2_SET);
				outl(1<<irq_nr, INTC0_CONFIG1_SET);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				break;
			case INTC_INT_DISABLED: /* 0:0:0 */
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				break;
			default: /* disable the interrupt */
				printk("unexpected int type %d (irq %d)\n", type, irq_nr);
				outl(1<<irq_nr, INTC0_CONFIG0_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG1_CLEAR);
				outl(1<<irq_nr, INTC0_CONFIG2_CLEAR);
				return;
		}
		if (int_req) /* assign to interrupt request 1 */
			outl(1<<irq_nr, INTC0_ASSIGN_REQ_CLEAR);
		else	     /* assign to interrupt request 0 */
			outl(1<<irq_nr, INTC0_ASSIGN_REQ_SET);
		outl(1<<irq_nr, INTC0_SOURCE_SET);
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
	}
}

static unsigned int startup_au1000_irq(unsigned int irq_nr)
{
	return 0; 
}

static void shutdown_au1000_irq(unsigned int irq_nr)
{
	/* *really* disable this interrupt */
	disable_au1000_irq(irq_nr);
	setup_au1000_irq(irq_nr, INTC_INT_DISABLED, 0);
	return;
}

static void enable_au1000_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_MASK_SET);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_SET);
	}
}

void enable_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_MASK_SET);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_SET);
	}
}

static void disable_au1000_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_MASK_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
	}
}

void disable_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_MASK_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_MASK_CLEAR);
	}
}

void ack_rise_edge_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_R_EDGE_DETECT_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_R_EDGE_DETECT_CLEAR);
	}
}

static void ack_fall_edge_irq(unsigned int irq_nr)
{
	if (irq_nr > AU1000_LAST_INTC0_INT) {
		outl(1<<irq_nr, INTC1_R_EDGE_DETECT_CLEAR);
	}
	else {
		outl(1<<irq_nr, INTC0_R_EDGE_DETECT_CLEAR);
	}
}

static void ack_level_irq(unsigned int irq_nr)
{
	return;
}

static void end_au1000_irq(unsigned int irq_nr)
{
	if (!(irq_desc[irq_nr].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_au1000_irq(irq_nr);
}


static struct hw_interrupt_type rise_edge_irq_type = {
	"Au1000 Rise Edge",
	startup_au1000_irq,
	shutdown_au1000_irq,
	enable_au1000_irq,
	disable_au1000_irq,
	ack_rise_edge_irq,
	end_au1000_irq,
	NULL
};

static struct hw_interrupt_type fall_edge_irq_type = {
	"Au1000 Edge",
	startup_au1000_irq,
	shutdown_au1000_irq,
	enable_au1000_irq,
	disable_au1000_irq,
	ack_fall_edge_irq,
	end_au1000_irq,
	NULL
};

static struct hw_interrupt_type level_irq_type = {
	"Au1000 Level",
	startup_au1000_irq,
	shutdown_au1000_irq,
	enable_au1000_irq,
	disable_au1000_irq,
	ack_level_irq,
	end_au1000_irq,
	NULL
};


int get_irq_list(char *buf)
{
	int i, len = 0, j;
	struct irqaction * action;

	len += sprintf(buf+len, "	    ");
	for (j=0; j<smp_num_cpus; j++)
		len += sprintf(buf+len, "CPU%d	     ",j);
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
			len += sprintf(buf+len, "  None	     ");
		len += sprintf(buf+len, "    %s",action->name);
		for (action=action->next; action; action = action->next) {
			len += sprintf(buf+len, ", %s", action->name);
		}
		len += sprintf(buf+len, "\n");
	}
	len += sprintf(buf+len, "BAD: %10lu\n", spurious_count);
	return len;
}

asmlinkage void do_IRQ(int irq, struct pt_regs *regs)
{
	struct irqaction *action;
	irq_desc_t *desc = irq_desc + irq;
	int cpu;

	cpu = smp_processor_id();
	irq_enter(cpu, irq);

	kstat.irqs[cpu][irq]++;
	desc->handler->ack(irq);

	action = irq_desc[irq].action;

	if (action && action->handler)
	{
		//desc->handler->disable(irq);
		//if (!(action->flags & SA_INTERRUPT)) __sti(); /* reenable ints */
		do { 
			action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while ( action );
		//__cli(); /* disable ints */
		//desc->handler->ack(irq);
		//desc->handler->enable(irq);
	}
	else
	{
		spurious_count++;
		printk("Unhandled interrupt %d, cause %x, disabled\n", 
				(unsigned)irq, (unsigned)regs->cp0_cause);
		desc->handler->disable(irq);
	}
	irq_exit(cpu, irq);
#if 0
	if (softirq_active(cpu) & softirq_mask(cpu))
		do_softirq();
#endif
}

int request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
	unsigned long irqflags, const char * devname, void *dev_id)
{
	struct irqaction *old, **p, *action;
	unsigned long flags, shared=0;
	irq_desc_t *desc = irq_desc + irq;
	unsigned long cp0_status;

	cp0_status = read_32bit_cp0_register(CP0_STATUS);

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
			desc->handler->disable(irq);
			desc->handler->ack(irq);
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
	
	action->handler = handler;
	action->flags = irqflags;					
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = NULL;

	p = &irq_desc[irq].action;
	
	spin_lock_irqsave(&desc->lock,flags);
	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & action->flags & SA_SHIRQ)) {
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
	*p = action;
	if (!shared) {
		desc->depth = 0;
		desc->status &= ~(IRQ_DISABLED | IRQ_AUTODETECT | IRQ_WAITING);
		desc->handler->startup(irq);
		desc->handler->ack(irq);
		desc->handler->enable(irq);
	}
	else {
		printk("irq %d is shared\n", irq);
	}
	spin_unlock_irqrestore(&desc->lock,flags);
	return 0;
}
		
void free_irq(unsigned int irq, void *dev_id)
{
	request_irq(irq, NULL, 0, NULL, dev_id);
}

void enable_cpu_timer(void)
{
	enable_cpu_irq_input(1<<MIPS_TIMER_IP); /* timer interrupt */
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
	int i;
	unsigned long cp0_status;

	cp0_status = read_32bit_cp0_register(CP0_STATUS);
	memset(irq_desc, 0, sizeof(irq_desc));
	set_except_vector(0, au1000_IRQ);

	for (i = 0; i <= NR_IRQS; i++) {
		irq_desc[i].status	= IRQ_DISABLED;
		irq_desc[i].action	= 0;
		irq_desc[i].depth	= 1;
		switch (i) {
#if 0
			case AU1000_MAC0_DMA_INT:
				setup_au1000_irq(i, INTC_INT_RISE_EDGE, 0);
				irq_desc[i].handler = &rise_edge_irq_type;
				break;
#endif
			default: /* active high, level interrupt */
				setup_au1000_irq(i, INTC_INT_HIGH_LEVEL, 0);
				disable_au1000_irq(i);
				irq_desc[i].handler = &level_irq_type;
				break;
		}
	}

	set_cp0_status(ALLINTS);
#ifdef CONFIG_REMOTE_DEBUG
	/* If local serial I/O used for debug port, enter kgdb at once */
	puts("Waiting for kgdb to connect...");
	set_debug_traps();
	breakpoint(); 
#endif
}


/*
 * Ack an int, in case we missed an interrupt ack somewhere.
 * This can be used for edge interrupts, in an error recovery
 * routine.
 */
void disable_ack_irq(int irq)
{
	irq_desc_t *desc = irq_desc + irq;

	desc->handler->disable(irq);
	desc->handler->ack(irq);
}

void mips_spurious_interrupt(struct pt_regs *regs)
{
#if 0
	return;
#else
	unsigned cause;

	cause = read_32bit_cp0_register(CP0_CAUSE);
	printk("spurious int (epc %x) (cause %x) (badvaddr %x)\n",
			(unsigned)regs->cp0_epc, cause, (unsigned)regs->cp0_badvaddr);
#endif
}

void intc0_req0_irqdispatch(struct pt_regs *regs)
{
	int irq = 0;
	unsigned long int_request;

	int_request = inl(INTC0_REQ0_INT);

	if (!int_request) return;

	for (;;) {
		if (!(int_request & 0x1)) {
			irq++;
			int_request >>= 1;
		}
		else break;
	}
	do_IRQ(irq, regs);
}

void intc0_req1_irqdispatch(struct pt_regs *regs)
{
	int irq = 0;
	unsigned long int_request;

	int_request = inl(INTC0_REQ1_INT);

	if (!int_request) return;

	for (;;) {
		if (!(int_request & 0x1)) {
			irq++;
			int_request >>= 1;
		}
		else break;
	}
	do_IRQ(irq, regs);
}

void intc1_req0_irqdispatch(struct pt_regs *regs)
{
	int irq = 0;
	unsigned long int_request;

	int_request = inl(INTC1_REQ0_INT);

	if (!int_request) return;

	for (;;) {
		if (!(int_request & 0x1)) {
			irq++;
			int_request >>= 1;
		}
		else break;
	}
	do_IRQ(irq, regs);
}

void intc1_req1_irqdispatch(struct pt_regs *regs)
{
	int irq = 0;
	unsigned long int_request;

	int_request = inl(INTC1_REQ1_INT);

	if (!int_request) return;

	for (;;) {
		if (!(int_request & 0x1)) {
			irq++;
			int_request >>= 1;
		}
		else break;
	}
	do_IRQ(irq, regs);
}

void show_pending_irqs(void)
{
}
