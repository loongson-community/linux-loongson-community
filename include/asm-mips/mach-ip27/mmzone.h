#ifndef _ASM_MACH_MMZONE_H
#define _ASM_MACH_MMZONE_H

#include <asm/sn/addrs.h>
#include <asm/sn/klkernvars.h>

typedef struct ip27_pglist_data {
	pg_data_t	gendata;
	kern_vars_t	kern_vars;
} ip27_pg_data_t;

extern ip27_pg_data_t *ip27_node_data[];

#define NODE_DATA(n)		(&(ip27_node_data[n]->gendata))
#define node_kern_vars(n)	(&(ip27_node_data[n]->kern_vars))
#define pa_to_nid(addr)		NASID_TO_COMPACT_NODEID(NASID_GET(addr))

#endif /* _ASM_MACH_MMZONE_H */
