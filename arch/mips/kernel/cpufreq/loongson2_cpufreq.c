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

#include <loongson.h>

/*
 * Percentages of cpu frequency (total levels = 8)
 *    Percentage    = index of clockmod_table[] / total levels
 *	25%		2
 *	50%		4
 *	100%		8
 */
#define DC_RESV	0
#define DC_25PT 2
#define DC_50PT 4
#define DC_100PT 8

/*
 * For Loongson's frequency is not high, we set the minimum level as 50% to
 * avoid spending too much time on freq switching
 */
static struct cpufreq_frequency_table clockmod_table[] = {
	{DC_RESV, CPUFREQ_ENTRY_INVALID},
	{DC_25PT, 0},
	{DC_50PT, 0},
	{DC_100PT, 0},
	{DC_RESV, CPUFREQ_TABLE_END},
};

static unsigned int max_cpufreq_khz;

static inline unsigned int idx_to_freq(unsigned int idx)
{
	/*
	 * freq = max_cpufreq_khz * (index / total levels)
	 *	= (max_cpufreq_khz * index) / 8
	 *	= (max_cpufreq_khz * index) >> 3
	 */
	return (max_cpufreq_khz * idx) >> 3;
}

/*
 * Map from index of clockmod_table to chipcfg:
 *	Index	lowest 3bits of Chipcfg
 *	8	7
 *	4	3
 *	2	1
 */
static unsigned int l2_cpufreq_get(unsigned int cpu)
{
	return idx_to_freq(LOONGSON_GET_CPUFREQ() + 1);
}

static void l2_cpufreq_set(unsigned int newstate)
{
	raw_spin_lock(&loongson_cpufreq_lock);
	LOONGSON_SET_CPUFREQ(clockmod_table[newstate].index - 1);
	raw_spin_unlock(&loongson_cpufreq_lock);
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

static int l2_cpu_freq_notifier(struct notifier_block *nb,
					unsigned long val, void *data)
{
	/* loops_per_jiffy is updated by adjust_jiffies() */
	if (val == CPUFREQ_POSTCHANGE)
		current_cpu_data.udelay_val = loops_per_jiffy;

	return 0;
}

static struct notifier_block l2_cpufreq_nb = {
	.notifier_call = l2_cpu_freq_notifier
};

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

static int __init l2_cpufreq_init(void)
{
	int ret;

	ret = cpufreq_register_notifier(&l2_cpufreq_nb,
			CPUFREQ_TRANSITION_NOTIFIER);
	if (ret)
		return ret;

	ret = cpufreq_register_driver(&l2_cpufreq_driver);
	if (ret) {
		cpufreq_unregister_notifier(&l2_cpufreq_nb,
				CPUFREQ_TRANSITION_NOTIFIER);
		return ret;
	}

	loongson_cpufreq_driver_loaded = 1;

	return 0;
}

static void __exit l2_cpufreq_exit(void)
{
	if (!loongson_cpufreq_driver_loaded)
		return;

	loongson_cpufreq_driver_loaded = 0;
	cpufreq_unregister_driver(&l2_cpufreq_driver);

	cpufreq_unregister_notifier(&l2_cpufreq_nb,
			CPUFREQ_TRANSITION_NOTIFIER);
}

module_init(l2_cpufreq_init);
module_exit(l2_cpufreq_exit);

MODULE_AUTHOR("Wu Zhangjin <wuzhangjin@gmail.com>");
MODULE_DESCRIPTION("cpufreq driver for Loongson-2");
MODULE_LICENSE("GPL");
