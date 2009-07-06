/*
 * CS5536 General timer functions
 *
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Yanhua, yanh@lemote.com
 *
 * Copyright (C) 2009 Lemote Inc. & Insititute of Computing Technology
 * Author: Wu zhangjin, wuzj@lemote.com
 *
 * Reference: 'AMD Geode(TM) CS5536 Companion Device Data Book'
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>

#include <asm/time.h>

#include <cs5536/cs5536_mfgpt.h>

DEFINE_SPINLOCK(mfgpt_lock);
EXPORT_SYMBOL(mfgpt_lock);

u32 mfgpt_base;

/*
 * Initialize the MFGPT timer.
 *
 * This is also called after resume to bring the MFGPT into operation again.
 */
/* setup register bit fields:
 * 15: counter enable
 * 14: compare2 output status, write 1 to clear when in event mode
 * 13: compare1 output status
 * 12: setup(ro)
 * 11: stop enable, stop on sleep
 * 10: external enable
 * 9:8 compare2 mode; 00: disable, 01: compare on equal; 10: compare on GE,
 * 	11 event: GE + irq
 * 7:6 compare1 mode
 * 5:  reverse enable, bit reverse of the counter
 * 4:  clock select. 0: 32KHz, 1: 14.318MHz
 * 3:0 counter prescaler scale factor.
 * 	select the input clock divide-by value. 2^n
 * bit 11:0 is write once
 */

static void init_mfgpt_timer(enum clock_event_mode mode,
			     struct clock_event_device *evt)
{
	spin_lock(&mfgpt_lock);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		outw(COMPARE, MFGPT0_CMP2);	/* set comparator2 */
		outw(0, MFGPT0_CNT);	/* set counter to 0 */
		/* enable counter, comparator2 to event mode, 14.318MHz clock */
		outw(0xe310, MFGPT0_SETUP);
		break;

	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:
		if (evt->mode == CLOCK_EVT_MODE_PERIODIC ||
		    evt->mode == CLOCK_EVT_MODE_ONESHOT) {
			/* disable counter */
			outw(inw(MFGPT0_SETUP) & 0x7fff, MFGPT0_SETUP);
		}
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		/* One shot setup */
		outw(0xe300, MFGPT0_SETUP);
		break;

	case CLOCK_EVT_MODE_RESUME:
		/* Nothing to do here */
		break;
	}
	spin_unlock(&mfgpt_lock);
}

/*
 * Program the next event in oneshot mode
 *
 * Delta is given in MFGPT ticks
 */
static int mfgpt_next_event(unsigned long delta, struct clock_event_device *evt)
{
	spin_lock(&mfgpt_lock);
	outw(delta & 0xffff, MFGPT0_CMP2);	/* set comparator2 */
	outw(0, MFGPT0_CNT);	/* set counter to 0 */
	spin_unlock(&mfgpt_lock);

	return 0;
}

static struct clock_event_device mfgpt_clockevent = {
	.name = "mfgpt",
	.features = CLOCK_EVT_FEAT_PERIODIC,
	.set_mode = init_mfgpt_timer,
	.set_next_event = mfgpt_next_event,
	.irq = CS5536_MFGPT_INTR,
};

static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	u32 basehi;

	/*
	 * get MFGPT base address
	 *
	 * NOTE: do not remove me, it's need for the value of mfgpt_base is
	 * variable
	 */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), &basehi, &mfgpt_base);

	/* ack */
	outw(inw(MFGPT0_SETUP) | 0x4000, MFGPT0_SETUP);

	mfgpt_clockevent.event_handler(&mfgpt_clockevent);

	return IRQ_HANDLED;
}

static struct irqaction irq5 = {
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING | IRQF_TIMER,
	.name = "timer"
};

/*
 * Initialize the conversion factor and the min/max deltas of the clock event
 * structure and register the clock event source with the framework.
 */
void __init setup_mfgpt_timer(void)
{
	u32 basehi;
	struct clock_event_device *cd = &mfgpt_clockevent;
	unsigned int cpu = smp_processor_id();

	cd->cpumask = cpumask_of(cpu);
	clockevent_set_clock(cd, MFGPT_TICK_RATE);
	cd->max_delta_ns = clockevent_delta2ns(0xffff, cd);
	cd->min_delta_ns = clockevent_delta2ns(0xf, cd);

	/* Enable MFGPT0 Comparator 2 Output to the Interrupt Mapper */
	_wrmsr(DIVIL_MSR_REG(MFGPT_IRQ), 0, 0x100);

	/* Enable Interrupt Gate 5 */
	_wrmsr(DIVIL_MSR_REG(PIC_ZSEL_LOW), 0, 0x50000);

	/* get MFGPT base address */
	_rdmsr(DIVIL_MSR_REG(DIVIL_LBAR_MFGPT), &basehi, &mfgpt_base);

	irq5.mask = cpumask_of_cpu(cpu);

	clockevents_register_device(cd);

	setup_irq(CS5536_MFGPT_INTR, &irq5);
}

/*
 * Since the MFGPT overflows every tick, its not very useful
 * to just read by itself. So use jiffies to emulate a free
 * running counter:
 */
static cycle_t mfgpt_read(struct clocksource *cs)
{
	unsigned long flags;
	int count;
	u32 jifs;
	static int old_count;
	static u32 old_jifs;

	spin_lock_irqsave(&mfgpt_lock, flags);
	/*
	 * Although our caller may have the read side of xtime_lock,
	 * this is now a seqlock, and we are cheating in this routine
	 * by having side effects on state that we cannot undo if
	 * there is a collision on the seqlock and our caller has to
	 * retry.  (Namely, old_jifs and old_count.)  So we must treat
	 * jiffies as volatile despite the lock.  We read jiffies
	 * before latching the timer count to guarantee that although
	 * the jiffies value might be older than the count (that is,
	 * the counter may underflow between the last point where
	 * jiffies was incremented and the point where we latch the
	 * count), it cannot be newer.
	 */
	jifs = jiffies;
	/* read the count */
	count = inw(MFGPT0_CNT);

	/*
	 * It's possible for count to appear to go the wrong way for this
	 * reason:
	 *
	 *  The timer counter underflows, but we haven't handled the resulting
	 *  interrupt and incremented jiffies yet.
	 *
	 * Previous attempts to handle these cases intelligently were buggy, so
	 * we just do the simple thing now.
	 */
	if (count < old_count && jifs == old_jifs)
		count = old_count;

	old_count = count;
	old_jifs = jifs;

	spin_unlock_irqrestore(&mfgpt_lock, flags);

	return (cycle_t) (jifs * COMPARE) + count;
}

static struct clocksource clocksource_mfgpt = {
	.name = "mfgpt",
	.rating = 120, /* Functional for real use, but not desired */
	.read = mfgpt_read,
	.mask = CLOCKSOURCE_MASK(32),
	.mult = 0,
	.shift = 22,
};

int __init init_mfgpt_clocksource(void)
{
	if (num_possible_cpus() > 1)	/* MFGPT does not scale! */
		return 0;

	clocksource_mfgpt.mult = clocksource_hz2mult(MFGPT_TICK_RATE, 22);
	return clocksource_register(&clocksource_mfgpt);
}

arch_initcall(init_mfgpt_clocksource);
