/* $Id: ip27-memory.c,v 1.2 2000/01/27 01:05:24 ralf Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 by Ralf Baechle
 * Copyright (C) 2000 by Silicon Graphics, Inc.
 *
 * On SGI IP27 the ARC memory configuration data is completly bogus but
 * alternate easier to use mechanisms are available.
 */
#include <linux/init.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

#include <asm/page.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/sn/types.h>
#include <asm/sn/addrs.h>
#include <asm/sn/klconfig.h>
#include <asm/sn/arch.h>

typedef unsigned long pfn_t;			/* into <asm/sn/types.h> */
#define KDM_TO_PHYS(x)	((x) & TO_PHYS_MASK)	/* into asm/addrspace.h */

#define SLOT_PFNSHIFT	(SLOT_SHIFT - PAGE_SHIFT) /* into asm/sn/arch.h */
#define PFN_NASIDSHFT	(NASID_SHFT - PAGE_SHIFT)
#define mkpfn(nasid, off)	(((pfn_t)(nasid) << PFN_NASIDSHFT) | (off))
#define slot_getbasepfn(node,slot) \
		(mkpfn(COMPACT_TO_NASID_NODEID(node), slot<<SLOT_PFNSHIFT))
extern nasid_t compact_to_nasid_node[];

extern char _end;

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

short slot_lastfilled_cache[MAX_COMPACT_NODES];
unsigned short slot_psize_cache[MAX_COMPACT_NODES][MAX_MEM_SLOTS];

/*
 * Return pfn of first free page of memory on a node. PROM may allocate
 * data structures on the first couple of pages of the first slot of each 
 * node. If this is the case, getfirstfree(node) > getslotstart(node, 0).
 */
pfn_t node_getfirstfree(cnodeid_t cnode)
{
#ifdef LATER
	nasid_t nasid = COMPACT_TO_NASID_NODEID(cnode);

	if (cnode == 0)
		return KDM_TO_PHYS((unsigned long)(&_end));
	return KDM_TO_PHYS(SYMMON_STK_ADDR(nasid, 0));
#endif
	if (cnode == 0)
		return (KDM_TO_PHYS(PFN_ALIGN(&_end) - (CKSEG0 - K0BASE)) 
								>> PAGE_SHIFT);
	return slot_getbasepfn(cnode, 0);
}

/*
 * Return the number of pages of memory provided by the given slot
 * on the specified node.
 */
pfn_t slot_getsize(cnodeid_t node, int slot)
{
	return (pfn_t) slot_psize_cache[node][slot];
}

/*
 * Return the pfn of the last free page of memory on a node.
 */
pfn_t node_getmaxclick(cnodeid_t node)
{
	pfn_t	slot_psize;
	int	slot;

	/*
	 * Start at the top slot. When we find a slot with memory in it,
	 * that's the winner.
	 */
	for (slot = (node_getnumslots(node) - 1); slot >= 0; slot--) {
		if ((slot_psize = slot_getsize(node, slot))) {
			/* Return the basepfn + the slot size, minus 1. */
			return slot_getbasepfn(node, slot) + slot_psize - 1;
		}
	}

	/*
	 * If there's no memory on the node, return 0. This is likely
	 * to cause problems.
	 */
	return (pfn_t)0;
}

static pfn_t slot_psize_compute(cnodeid_t node, int slot)
{
	nasid_t nasid;
	lboard_t *brd;
	klmembnk_t *banks;
	unsigned long size;

	nasid = COMPACT_TO_NASID_NODEID(node);
	/* Find the node board */
	brd = find_lboard_real((lboard_t *)KL_CONFIG_INFO(nasid), KLTYPE_IP27);
	if (!brd)
		return 0;

	/* Get the memory bank structure */
	banks = (klmembnk_t *)find_first_component(brd, KLSTRUCT_MEMBNK);
	if (!banks)
		return 0;

	/* Size in _Megabytes_ */
	size = (unsigned long)banks->membnk_bnksz[slot/4];

	/* hack for 128 dimm banks */
	if (size <= 128) {
		if (slot%4 == 0) {
			size <<= 20;		/* size in bytes */
			return(size >> PAGE_SHIFT);
		} else {
			return 0;
		}
	} else {
		size /= 4;
		size <<= 20;
		return(size >> PAGE_SHIFT);
	}
}

pfn_t szmem(pfn_t fpage, pfn_t maxpmem)
{
	int node, slot;
	int numslots;
	pfn_t num_pages = 0;
	pfn_t slot_psize;

	for (node = 0; node < numnodes; node++) {
		numslots = node_getnumslots(node);
		for (slot = 0; slot < numslots; slot++) {
			slot_psize = slot_psize_compute(node, slot);
			num_pages += slot_psize;
			slot_psize_cache[node][slot] = 
					(unsigned short) slot_psize;
			if (slot_psize)
				slot_lastfilled_cache[node] = slot;
		}
	}
	if (maxpmem)
		return((maxpmem > num_pages) ? num_pages : maxpmem);
	else
		return num_pages;
}

/*
 * Currently, the intranode memory hole support assumes that each slot
 * contains at least 32 MBytes of memory. We assume all bootmem data
 * fits on the first slot.
 */
void __init prom_meminit(void)
{
	extern void mlreset(void);
	cnodeid_t node;
	pfn_t slot_firstpfn, slot_lastpfn, slot_freepfn, numpages;
	unsigned long bootmap_size;

	mlreset();
	numpages = szmem(0, 0);
	for (node = (numnodes - 1); node >= 0; node--) {
		slot_firstpfn = slot_getbasepfn(node, 0);
		slot_lastpfn = slot_firstpfn + slot_getsize(node, 0);
		slot_freepfn = node_getfirstfree(node);
		max_low_pfn = (slot_lastpfn - slot_firstpfn);
	  	bootmap_size = init_bootmem_node(node, slot_freepfn, 
						slot_firstpfn, slot_lastpfn);
		free_bootmem_node(node, slot_firstpfn << PAGE_SHIFT, 
				(slot_lastpfn - slot_firstpfn) << PAGE_SHIFT);
		reserve_bootmem_node(node, slot_firstpfn << PAGE_SHIFT,
		  ((slot_freepfn - slot_firstpfn) << PAGE_SHIFT) + bootmap_size);
	}
	printk("Total memory probed : 0x%lx pages\n", numpages);
}

int __init page_is_ram(unsigned long pagenr)
{
        return 1;
}

void __init
prom_free_prom_memory (void)
{
	/* We got nothing to free here ...  */
}

#ifdef CONFIG_DISCONTIGMEM

plat_pg_data_t plat_node_data[MAX_COMPACT_NODES];

int numa_debug(void)
{
	printk("NUMA debug\n");
	*(int *)0 = 0;
	return(0);
}
#endif
