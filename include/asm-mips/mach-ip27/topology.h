#ifndef _ASM_MACH_TOPOLOGY_H
#define _ASM_MACH_TOPOLOGY_H	1

#include <asm/sn/arch.h>

#define cpu_to_node(cpu)	(cpu_data[(cpu)].p_nodeid)
#define parent_node(node)	(0)
#define node_to_cpumask(node)	(cpu_online_map)
#define node_to_first_cpu(node)	(NODEPDA(_cnode)->node_first_cpu)
#define pcibus_to_cpumask(bus)	(cpu_online_map)

/* Cross-node load balancing interval. */
#define NODE_BALANCE_RATE	10

#endif /* _ASM_MACH_TOPOLOGY_H */
