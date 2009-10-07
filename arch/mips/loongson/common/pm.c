/*
 * loongson-specific STR/Standby support
 *
 *  Copyright (C) 2009 Lemote Inc. & Insititute of Computing Technology
 *  Author: Wu Zhangjin <wuzj@lemote.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/pm.h>

#include <asm/i8259.h>
#include <asm/delay.h>
#include <asm/mipsregs.h>

#include <loongson.h>

#include <cs5536/cs5536_mfgpt.h>

static unsigned int cached_master_mask;	/* i8259A */
static unsigned int cached_slave_mask;
static unsigned int cached_bonito_irq_mask; /* bonito */

void arch_suspend_disable_irqs(void)
{
	/* disable all mips eventss */
	local_irq_disable();

	/* disable all eventss of i8259A */
	cached_slave_mask = inb(PIC_SLAVE_IMR);
	cached_master_mask = inb(PIC_MASTER_IMR);

	outb(0xff, PIC_SLAVE_IMR);
	inb(PIC_SLAVE_IMR);
	outb(0xff, PIC_MASTER_IMR);
	inb(PIC_MASTER_IMR);

	/* disable all eventss of bonito */
	cached_bonito_irq_mask = LOONGSON_INTEN;
	LOONGSON_INTENCLR = 0xffff;
	(void)LOONGSON_INTENCLR;
}

void arch_suspend_enable_irqs(void)
{
	/* enable all mips eventss */
	local_irq_enable();

	/* only enable the cached eventss of i8259A */
	outb(cached_slave_mask, PIC_SLAVE_IMR);
	outb(cached_master_mask, PIC_MASTER_IMR);

	/* enable all cached eventss of bonito */
	LOONGSON_INTENSET = cached_bonito_irq_mask;
	(void)LOONGSON_INTENSET;
}

/* setup the board-specific events for waking up loongson from wait mode */
void __attribute__((weak)) setup_wakeup_events(void)
{
}

/* check wakeup events */
int __attribute__((weak)) wakeup_loongson(void)
{
	return 1;
}

/* if the events are really what we want to wakeup cpu, wake up it, otherwise,
 * we Put CPU into wait mode again.
 */
static void wait_for_wakeup_events(void)
{
	while (!wakeup_loongson())
		LOONGSON_CHIPCFG0 &= ~0x7;
}

/* stop all perf counters by default
 *   $24 is the control register of loongson perf counter
 */
static inline void stop_perf_counters(void)
{
	__write_64bit_c0_register($24, 0, 0);
}


static void loongson_suspend_enter(void)
{
	static unsigned int cached_cpu_freq;

	/* setup wakeup events via enabling the IRQs */
	setup_wakeup_events();

	/* stop all perf counters */
	stop_perf_counters();

	cached_cpu_freq = LOONGSON_CHIPCFG0;

	/* Put CPU into wait mode */
	LOONGSON_CHIPCFG0 &= ~0x7;

	/* wait for the given events to wakeup cpu from wait mode */
	wait_for_wakeup_events();

	LOONGSON_CHIPCFG0 = cached_cpu_freq;
	mmiowb();
}

void __attribute__((weak)) mach_suspend(void)
{
	disable_mfgpt0_counter();
}

void __attribute__((weak)) mach_resume(void)
{
	enable_mfgpt0_counter();
}

static int loongson_pm_enter(suspend_state_t state)
{
	/* mach specific suspend */
	mach_suspend();

	/* processor specific suspend */
	loongson_suspend_enter();

	/* mach specific resume */
	mach_resume();

	return 0;
}

static int loongson_pm_valid_state(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;

	default:
		return 0;
	}
}

static struct platform_suspend_ops loongson_pm_ops = {
	.valid	= loongson_pm_valid_state,
	.enter	= loongson_pm_enter,
};

static int __init loongson_pm_init(void)
{
	suspend_set_ops(&loongson_pm_ops);

	return 0;
}
arch_initcall(loongson_pm_init);
