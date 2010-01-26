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

#include <asm/time.h>

#ifdef CONFIG_CPU_SUPPORTS_HR_SCHED_CLOCK
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

unsigned long long notrace sched_clock(void)
{
	unsigned long long v = cnt32_to_63(read_c0_count());
	return (v * clock2ns_scale) >> CLOCK2NS_SCALE_FACTOR;
}

static struct timer_list cnt32_to_63_keepwarm_timer;

static void cnt32_to_63_keepwarm(unsigned long data)
{
	mod_timer(&cnt32_to_63_keepwarm_timer, round_jiffies(jiffies + data));
	sched_clock();
}
#endif

static inline void setup_hres_sched_clock(unsigned long clock)
{
#ifdef CONFIG_CPU_SUPPORTS_HR_SCHED_CLOCK
	unsigned long long v;
	unsigned long data;

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

	data = 0x80000000UL / clock * HZ;
	setup_timer(&cnt32_to_63_keepwarm_timer, cnt32_to_63_keepwarm, data);
	mod_timer(&cnt32_to_63_keepwarm_timer, round_jiffies(jiffies + data));
#endif
}

static cycle_t c0_hpt_read(struct clocksource *cs)
{
	return read_c0_count();
}

static struct clocksource clocksource_mips = {
	.name		= "MIPS",
	.read		= c0_hpt_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

int __init init_r4k_clocksource(void)
{
	if (!cpu_has_counter || !mips_hpt_frequency)
		return -ENXIO;

	setup_hres_sched_clock(mips_hpt_frequency);

	/* Calculate a somewhat reasonable rating value */
	clocksource_mips.rating = 200 + mips_hpt_frequency / 10000000;

	clocksource_set_clock(&clocksource_mips, mips_hpt_frequency);

	clocksource_register(&clocksource_mips);

	return 0;
}
