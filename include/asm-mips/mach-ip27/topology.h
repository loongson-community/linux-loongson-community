#ifndef _ASM_MACH_TOPOLOGY_H
#define _ASM_MACH_TOPOLOGY_H	1

#include <asm/sn/hub.h>

#define cpu_to_node(cpu)	(cpu_data[(cpu)].p_nodeid)
#define parent_node(node)	(node)
#define node_to_cpumask(node)	(HUB_DATA(node)->h_cpus)
#define node_to_first_cpu(node)	(first_cpu(node_to_cpumask(node)))
#define pcibus_to_cpumask(bus)	(cpu_online_map)

extern int node_distance(nasid_t nasid_a, nasid_t nasid_b);
#define node_distance(from, to)	node_distance(from, to)

/* sched_domains SD_NODE_INIT for SGI IP27 machines */
#define SD_NODE_INIT (struct sched_domain) {		\
	.span			= CPU_MASK_NONE,	\
	.parent			= NULL,			\
	.groups			= NULL,			\
	.min_interval		= 8,			\
	.max_interval		= 32,			\
	.busy_factor		= 32,			\
	.imbalance_pct		= 125,			\
	.cache_hot_time		= (10*1000),		\
	.cache_nice_tries	= 1,			\
	.per_cpu_gain		= 100,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_EXEC	\
				| SD_WAKE_BALANCE,	\
	.last_balance		= jiffies,		\
	.balance_interval	= 1,			\
	.nr_balance_failed	= 0,			\
}

#endif /* _ASM_MACH_TOPOLOGY_H */
