/*
 * Cpufreq driver for the loongson-2 (>= 2F) processors
 *
 * Copyright (C) 2010, Wu Zhangjin <wuzhangjin@gmail.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

#include <asm/mach-loongson/loongson.h>

#define DC_RESV	0

/*
 * For Loongson's frequency is not high, we set the minimum level as 50% to
 * avoid spending too much time on freq switching
 */
static struct cpufreq_frequency_table clockmod_table[] = {
	{DC_RESV, CPUFREQ_ENTRY_INVALID},
	{1, 0},
	{3, 0},
	{7, 0},
	{DC_RESV, CPUFREQ_TABLE_END},
};

static unsigned int max_cpufreq_khz;

static inline unsigned int idx_to_freq(unsigned int idx)
{
	/*
	 * freq = max_cpufreq_khz * ((index + 1) / total levels)
	 *	= (max_cpufreq_khz * (index + 1)) / 8
	 *	= (max_cpufreq_khz * (index + 1)) >> 3
	 */
	return (max_cpufreq_khz * (idx + 1)) >> 3;
}

static inline unsigned int l2_cpufreq_get(unsigned int cpu)
{
	return idx_to_freq(LOONGSON_GET_CPUFREQ());
}

static inline unsigned int idx_to_scale_shift(unsigned int newstate)
{

	/*
	 * newstate the the index of the array clockmod_table, the valid value
	 * is 1, 2, 3.
	 *
	 * The return value is the scale_shift for respective frequency.
	 *
	 *  newstate | Freq_scale of CR80 | multiple        | scale_shift
	 *     1     |        1           | 8 / (1+1) = 4   |  2
	 *     2     |        3           | 8 / (3+1) = 2   |  1
	 *     3     |        7           | 8 / (7+1) = 1   |  0
	 *
	 *  scale_shift = 3 - newstate
	 */

	return 3 - newstate;
}

#ifdef CONFIG_R4K_TIMER_FOR_CPUFREQ
extern unsigned int scale_shift;
extern void update_virtual_count(unsigned int target_scale_shift);

static inline void sync_virtual_count(unsigned int target_scale_shift)
{
	update_virtual_count(target_scale_shift);
	scale_shift = target_scale_shift;
}

static void notrace l2_cpufreq_set(unsigned int newstate)
{
	unsigned long flag;
	unsigned int target_scale_shift;

	target_scale_shift = idx_to_scale_shift(newstate);

	pr_debug("%s: scale_shift = %d, target_scale_shift = %d, target_set: %d\n",
			__func__, scale_shift, target_scale_shift,
			clockmod_table[newstate].index);

	/* For we are UP, Give up the spin lock... */
	raw_local_irq_save(flag);
	/* When freq becomes higher ... */
	if (scale_shift > target_scale_shift)
		sync_virtual_count(target_scale_shift);
	/* Set the CR80 register */
	LOONGSON_SET_CPUFREQ(clockmod_table[newstate].index);
	/* When freq becomes lower ... */
	if (scale_shift < target_scale_shift)
		sync_virtual_count(target_scale_shift);
	raw_local_irq_restore(flag);

	pr_debug("%s: scale_shift = %d, target_scale_shift = %d, target_set: %d\n",
			__func__, scale_shift, target_scale_shift,
			clockmod_table[newstate].index);
}

/*
 * The CPUFreq driver will put the cpu into the lowest level(1), no need to do
 * it here.  If we do it here, some CPUFreq governors will not function well,
 * so, disable the cpu_wait() completely when the R4K is used.
 */

#if 0
/*
 * Put CPU into the 1st level, We have no good method to recover the timesplice
 * in wait mode, so, we only allow the CPU gointo the 1st level, not the ZERO
 * level.
 *
 * To avoid recording the garbage result in the kernel tracing, we don't call
 * notifiers when FUNCTION_TRACER is enabled.
 */

void notrace loongson2_cpu_wait(void)
{
#ifdef CONFIG_FUNCTION_TRACER
	/* If we are already in the 1st level, stop resetting it. */
	if (LOONGSON_GET_CPUFREQ() != 1)
		l2_cpufreq_set(1);
#else
	{
	struct cpufreq_freqs freqs;

	freqs.old = l2_cpufreq_get(0);
	freqs.new = idx_to_freq(1);

	if (freqs.new == freqs.old)
		return;

	/* notifiers */
	freqs.cpu = 0;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* setting the cpu frequency as the 1st level */
	l2_cpufreq_set(1);

	/* notifiers */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}
#endif
}
#else
#define loongson2_cpu_wait NULL
#endif

#else	/* MIPS_EXTERNAL_TIMER */

static void l2_cpufreq_set(unsigned int newstate)
{
	unsigned long flags;

	local_irq_save(flags);
	LOONGSON_SET_CPUFREQ(clockmod_table[newstate].index);
	local_irq_restore(flags);
}

static void notrace loongson2_cpu_wait(void);

#endif /* CONFIG_R4K_TIMER_FOR_CPUFREQ */

static inline void register_cpu_wait(void)
{
	cpu_wait = loongson2_cpu_wait;
}
static inline void unregister_cpu_wait(void)
{
	cpu_wait = NULL;
}

static int l2_cpufreq_target(struct cpufreq_policy *policy, unsigned int
		target_freq, unsigned int relation)
{
	unsigned int newstate;
	struct cpufreq_freqs freqs;

	if (cpufreq_frequency_table_target(policy, &clockmod_table[0],
				target_freq, relation, &newstate))
		return -EINVAL;

	freqs.old = l2_cpufreq_get(policy->cpu);
	freqs.new = idx_to_freq(clockmod_table[newstate].index);

	if (freqs.new == freqs.old)
		return 0;

	/* notifiers */
	freqs.cpu = policy->cpu;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

	/* setting the cpu frequency */
	l2_cpufreq_set(newstate);

	/* notifiers */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	return 0;
}

static int l2_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	int i;

	/* get max cpu frequency in khz */
	max_cpufreq_khz = cpu_clock_freq / 1000;

	/* table init */
	for (i = 1; clockmod_table[i].index != DC_RESV; i++)
		clockmod_table[i].frequency = idx_to_freq(clockmod_table[i].index);

	cpufreq_frequency_table_get_attr(clockmod_table, policy->cpu);

	/* cpuinfo and default policy values */

	policy->cur = max_cpufreq_khz;

	return cpufreq_frequency_table_cpuinfo(policy, &clockmod_table[0]);
}

static int l2_cpufreq_verify(struct cpufreq_policy *policy)
{
	return cpufreq_frequency_table_verify(policy, &clockmod_table[0]);
}

static int l2_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

static struct freq_attr *clockmod_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver l2_cpufreq_driver = {
	.owner = THIS_MODULE,
	.name = "l2_cpufreq",
	.init = l2_cpufreq_cpu_init,
	.exit = l2_cpufreq_cpu_exit,
	.verify = l2_cpufreq_verify,
	.target = l2_cpufreq_target,
	.get = l2_cpufreq_get,
	.attr = clockmod_attr,
};

static struct platform_device_id platform_device_ids[] = {
	{
		.name = "l2_cpufreq",
	},
	{}
};
MODULE_DEVICE_TABLE(platform, platform_device_ids);

static struct platform_driver platform_driver = {
	.driver = {
		.name = "l2_cpufreq",
		.owner = THIS_MODULE,
	},
	.id_table = platform_device_ids,
};

/*
 * This is the simple version of Loongson-2 wait, Maybe we need do this in
 * interrupt disabled context.
 */

static DEFINE_SPINLOCK(loongson2_wait_lock);

static void loongson2_cpu_wait(void)
{
	unsigned long flags;
	u32 cpu_freq;

	spin_lock_irqsave(&loongson2_wait_lock, flags);
	cpu_freq = LOONGSON_CHIPCFG0;
	LOONGSON_CHIPCFG0 &= ~0x7;      /* Put CPU into wait mode */
	LOONGSON_CHIPCFG0 = cpu_freq;   /* Restore CPU state */
	spin_unlock_irqrestore(&loongson2_wait_lock, flags);
}

static int __init l2_cpufreq_init(void)
{
	int ret;

	/* Register platform stuff */
	ret = platform_driver_register(&platform_driver);
	if (ret)
		return ret;

	ret = cpufreq_register_driver(&l2_cpufreq_driver);
	if (ret) {
		platform_driver_unregister(&platform_driver);
		return ret;
	}

	register_cpu_wait();

	return 0;
}

static void __exit l2_cpufreq_exit(void)
{
	unregister_cpu_wait();

	cpufreq_unregister_driver(&l2_cpufreq_driver);

	platform_driver_unregister(&platform_driver);
}

module_init(l2_cpufreq_init);
module_exit(l2_cpufreq_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzhangjin@gmail.com>");
MODULE_DESCRIPTION("cpufreq driver for Loongson-2");
MODULE_LICENSE("GPL");
