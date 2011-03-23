/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 by Ralf Baechle
 */
#include <linux/clocksource.h>
#include <linux/cnt32_to_63.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/module.h>

#include <asm/cpu-features.h>
#include <asm/time.h>

#include <loongson.h>

/*
 * Some MIPS cpu can change their frequency, meaning that read_c0_count doesn't
 * run at the same speed :(
 * Have to handle this case.
 */
#ifdef CONFIG_R4K_TIMER_FOR_CPUFREQ

unsigned int scale_shift;
EXPORT_SYMBOL(scale_shift);
#define hpt_scale_up(cycle) ((cycle) << scale_shift)

/*
 * read_virtual_count -- read the virtual 64bit count
 *
 * This should be called with irq disabled and spin lock
 *
 * @now: This should be read from the real count register
 * Return the virtual but precise 64bit count
 */

static u32 hpt_last_read;

cycle_t notrace read_virtual_count(void)
{
	static u64 hpt_last_cnt;
	u64 diff;
	unsigned int now;
	unsigned long flags;

	local_irq_save(flags);

	now = read_c0_count();

	if (unlikely(!hpt_last_read)) {
		/*
		 * The '1st' time c0_hpt_read() is called so, the cycle read
		 * from the counter is the real one
		 *
		 * When resuming from PM, we also reset it.
		 */
		hpt_last_cnt = hpt_last_read = now;
	} else {
		/* Get diff and Check for counter overflow */
		diff = (now - hpt_last_read) & CLOCKSOURCE_MASK(32);
		/* Calculate the real cycles */
		hpt_last_cnt += hpt_scale_up(diff);
		/* Save for the next access */
		hpt_last_read = now;
	}

	local_irq_restore(flags);

	return hpt_last_cnt;
}

static inline void reset_virtual_count(void)
{
	hpt_last_read = 0;
}

static void r4k_suspend(struct clocksource *cs)
{
}

static void r4k_resume(struct clocksource *cs)
{
	reset_virtual_count();
}

#define hpt_read() read_virtual_count()
#else
#define setup_r4k_for_cpufreq(clock)
#define hpt_read() read_c0_count()
#define r4k_suspend NULL
#define r4k_resume NULL
#endif	/* CONFIG_R4K_TIMER_FOR_CPUFREQ */

cycle_t notrace c0_hpt_read(struct clocksource *cs)
{
	return hpt_read();
}

/*
 * MIPS sched_clock implementation.
 *
 * Because the hardware timer period is quite short and because cnt32_to_63()
 * needs to be called at least once per half period to work properly, a kernel
 * timer is set up to ensure this requirement is always met.
 *
 * Please refer to include/linux/cnt32_to_63.h and arch/arm/plat-orion/time.c
 */
#define CLOCK2NS_SCALE_FACTOR 8

static unsigned long clock2ns_scale __read_mostly;

#ifdef CONFIG_R4K_FOR_CPUFREQ
#define hpt_read64() read_virtual_count()
#else
#define hpt_read64() cnt32_to_63(read_c0_count())
#endif
unsigned long long notrace sched_clock(void)
{
	return (hpt_read64() * clock2ns_scale) >> CLOCK2NS_SCALE_FACTOR;
}

#ifndef CONFIG_R4K_TIMER_FOR_CPUFREQ
static struct timer_list cnt32_to_63_keepwarm_timer;

static void cnt32_to_63_keepwarm(unsigned long data)
{
	mod_timer(&cnt32_to_63_keepwarm_timer, round_jiffies(jiffies + data));
	sched_clock();
}
#endif

static inline void setup_hres_sched_clock(unsigned long clock)
{
	unsigned long long v;
#ifndef CONFIG_R4K_TIMER_FOR_CPUFREQ
	unsigned long data;
#endif

	v = NSEC_PER_SEC;
	v <<= CLOCK2NS_SCALE_FACTOR;
	v += clock/2;
	do_div(v, clock);
	/*
	 * We want an even value to automatically clear the top bit
	 * returned by cnt32_to_63() without an additional run time
	 * instruction. So if the LSB is 1 then round it up.
	 */
	if (v & 1)
		v++;
	clock2ns_scale = v;
#ifndef CONFIG_R4K_TIMER_FOR_CPUFREQ
	data = 0x80000000UL / clock * HZ;
	setup_timer(&cnt32_to_63_keepwarm_timer, cnt32_to_63_keepwarm, data);
	mod_timer(&cnt32_to_63_keepwarm_timer, round_jiffies(jiffies + data));
#endif
}

static struct clocksource clocksource_mips = {
	.name		= "MIPS",
	.read		= c0_hpt_read,
#ifdef CONFIG_R4K_FOR_CPUFREQ
	.mask		= CLOCKSOURCE_MASK(64);
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS | CLOCK_SOURCE_MUST_VERIFY,
#else
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
#endif
#ifdef CONFIG_PM
	.suspend	= r4k_suspend,
	.resume		= r4k_resume,
#endif
};

static u64 r4k_udelay_factor __read_mostly;
static u64 r4k_ndelay_factor __read_mostly;

static inline void r4k_setup_delays(void)
{
	r4k_udelay_factor = mips_hpt_frequency;
	do_div(r4k_udelay_factor, 1000000);
	/*
	 * For __ndelay we divide by 2^16, so the factor is multiplied
	 * by the same amount.
	 */
	r4k_ndelay_factor = (r4k_udelay_factor * 0x10000ull);
	do_div(r4k_ndelay_factor, 1000ull);

	lpj_fine = mips_hpt_frequency / HZ;
}

static inline void rep_nop(void)
{
	__asm__ __volatile__("nop;" : : : "memory");
}

void __delay(unsigned int loops)
{
	cycle_t now, bclock;

	preempt_disable();
	bclock = hpt_read64();
	for (;;) {
		now = hpt_read64();
		if ((now - bclock) >= loops)
			break;
		/* Allow RT tasks to run */
		preempt_enable();
		rep_nop();
		preempt_disable();
	}
	preempt_enable();
}
EXPORT_SYMBOL(__delay);

void __udelay(unsigned int us)
{
	__delay(us * r4k_udelay_factor);
}
EXPORT_SYMBOL(__udelay);

void __ndelay(unsigned int ns)
{
	__delay((ns * r4k_ndelay_factor) >> 16);
}
EXPORT_SYMBOL(__ndelay);

int __init init_r4k_clocksource(void)
{
	if (!cpu_has_counter || !mips_hpt_frequency)
		return -ENXIO;

	r4k_setup_delays();

	setup_hres_sched_clock(mips_hpt_frequency);

	/* Calculate a somewhat reasonable rating value */
	clocksource_mips.rating = 200 + mips_hpt_frequency / 10000000;

	clocksource_set_clock(&clocksource_mips, mips_hpt_frequency);

	clocksource_register(&clocksource_mips);

	return 0;
}
