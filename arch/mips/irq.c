/*
 *	linux/kernel/irq.c
 *
 *	Copyright (C) 1992 Linus Torvalds
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQ's should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 */

/*
 * IRQ's are in fact implemented a bit like signal handlers for the kernel.
 * The same sigaction struct is used, and with similar semantics (ie there
 * is a SA_INTERRUPT flag etc). Naturally it's not a 1:1 relation, but there
 * are similarities.
 *
 * sa_handler(int irq_NR) is the default function called (0 if no).
 * sa_mask is horribly ugly (I won't even mention it)
 * sa_flags contains various info: SA_INTERRUPT etc
 * sa_restorer is the unused
 */

#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

#define CR0_NE 32

unsigned char cache_21 = 0xff;
unsigned char cache_A1 = 0xff;

unsigned long intr_count = 0;
unsigned long spurious_count = 0;
unsigned long bh_active = 0;
unsigned long bh_mask = 0xFFFFFFFF;
struct bh_struct bh_base[32]; 

void disable_irq(unsigned int irq_nr)
{
	unsigned long flags;
	unsigned char mask;

	mask = 1 << (irq_nr & 7);
	save_flags(flags);
	if (irq_nr < 8) {
		cli();
		cache_21 |= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
	}
	cli();
	cache_A1 |= mask;
	outb(cache_A1,0xA1);
	restore_flags(flags);
}

void enable_irq(unsigned int irq_nr)
{
	unsigned long flags;
	unsigned char mask;

	mask = ~(1 << (irq_nr & 7));
	save_flags(flags);
	if (irq_nr < 8) {
		cli();
		cache_21 &= mask;
		outb(cache_21,0x21);
		restore_flags(flags);
		return;
	}
	cli();
	cache_A1 &= mask;
	outb(cache_A1,0xA1);
	restore_flags(flags);
}

/*
 * do_bottom_half() runs at normal kernel priority: all interrupts
 * enabled.  do_bottom_half() is atomic with respect to itself: a
 * bottom_half handler need not be re-entrant.
 */
asmlinkage void do_bottom_half(void)
{
	unsigned long active;
	unsigned long mask, left;
	struct bh_struct *bh;

	bh = bh_base;
	active = bh_active & bh_mask;
	for (mask = 1, left = ~0 ; left & active ; bh++,mask += mask,left += left) {
		if (mask & active) {
			void (*fn)(void *);
			bh_active &= ~mask;
			fn = bh->routine;
			if (!fn)
				goto bad_bh;
			fn(bh->data);
		}
	}
	return;
bad_bh:
	printk ("irq.c:bad bottom half entry\n");
}

/*
 * Pointers to the low-level handlers: first the general ones, then the
 * fast ones, then the bad ones.
 */
extern void IRQ(void);
extern void fast_IRQ(void);
extern void bad_IRQ(void);

/*
 * Initial irq handlers.
 */
static struct sigaction irq_sigaction[16] = {
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL },
	{ NULL, 0, 0, NULL }, { NULL, 0, 0, NULL }
};

int get_irq_list(char *buf)
{
	int i, len = 0;
	struct sigaction * sa = irq_sigaction;

	for (i = 0 ; i < 16 ; i++, sa++) {
		if (!sa->sa_handler)
			continue;
		len += sprintf(buf+len, "%2d: %8d %c %s\n",
			i, kstat.interrupts[i],
			(sa->sa_flags & SA_INTERRUPT) ? '+' : ' ',
			(char *) sa->sa_mask);
	}
	return len;
}

/*
 * do_IRQ handles IRQ's that have been installed without the
 * SA_INTERRUPT flag: it uses the full signal-handling return
 * and runs with other interrupts enabled. All relatively slow
 * IRQ's should use this format: notably the keyboard/timer
 * routines.
 */
asmlinkage void do_IRQ(int irq, struct pt_regs * regs)
{
	struct sigaction * sa = irq + irq_sigaction;

	kstat.interrupts[irq]++;
	sa->sa_handler((int) regs);
}

/*
 * do_fast_IRQ handles IRQ's that don't need the fancy interrupt return
 * stuff - the handler is also running with interrupts disabled unless
 * it explicitly enables them later.
 */
asmlinkage void do_fast_IRQ(int irq)
{
	struct sigaction * sa = irq + irq_sigaction;

	kstat.interrupts[irq]++;
	sa->sa_handler(irq);
}

/*
 * Using "struct sigaction" is slightly silly, but there
 * are historical reasons and it works well, so..
 */
static int irqaction(unsigned int irq, struct sigaction * new_sa)
{
	struct sigaction * sa;
	unsigned long flags;

	if (irq > 15)
		return -EINVAL;
	sa = irq + irq_sigaction;
	if (sa->sa_handler)
		return -EBUSY;
	if (!new_sa->sa_handler)
		return -EINVAL;
	save_flags(flags);
	cli();
	*sa = *new_sa;
	/*
	 * FIXME: Does the SA_INTERRUPT flag make any sense on the MIPS???
	 */
	if (sa->sa_flags & SA_INTERRUPT)
		set_intr_gate(irq,fast_IRQ);
	else
		set_intr_gate(irq,IRQ);
	if (irq < 8) {
		cache_21 &= ~(1<<irq);
		outb(cache_21,0x21);
	} else {
		cache_21 &= ~(1<<2);
		cache_A1 &= ~(1<<(irq-8));
		outb(cache_21,0x21);
		outb(cache_A1,0xA1);
	}
	restore_flags(flags);
	return 0;
}

int request_irq(unsigned int irq, void (*handler)(int),
	unsigned long flags, const char * devname)
{
	struct sigaction sa;

	sa.sa_handler = handler;
	sa.sa_flags = flags;
	sa.sa_mask = (unsigned long) devname;
	sa.sa_restorer = NULL;
	return irqaction(irq,&sa);
}

void free_irq(unsigned int irq)
{
	struct sigaction * sa = irq + irq_sigaction;
	unsigned long flags;

	if (irq > 15) {
		printk("Trying to free IRQ%d\n",irq);
		return;
	}
	if (!sa->sa_handler) {
		printk("Trying to free free IRQ%d\n",irq);
		return;
	}
	save_flags(flags);
	cli();
	if (irq < 8) {
		cache_21 |= 1 << irq;
		outb(cache_21,0x21);
	} else {
		cache_A1 |= 1 << (irq-8);
		outb(cache_A1,0xA1);
	}
	set_intr_gate(irq,bad_IRQ);
	sa->sa_handler = NULL;
	sa->sa_flags = 0;
	sa->sa_mask = 0;
	sa->sa_restorer = NULL;
	restore_flags(flags);
}

#if 0
/*
 * handle fpa errors
 */
static void math_error_irq(int cpl)
{
	if (!hard_math)
		return;
	handle_fpe();
}
#endif

static void no_action(int cpl) { }

void init_IRQ(void)
{
	int i;

	for (i = 0; i < 16 ; i++)
		set_intr_gate(i, bad_IRQ[i]);
	if (request_irq(2, no_action, SA_INTERRUPT, "cascade"))
		printk("Unable to get IRQ2 for cascade\n");

	/* initialize the bottom half routines. */
	for (i = 0; i < 32; i++) {
		bh_base[i].routine = NULL;
		bh_base[i].data = NULL;
	}
	bh_active = 0;
	intr_count = 0;
}
