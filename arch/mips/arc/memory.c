/*
 * memory.c: PROM library functions for acquiring/using memory descriptors
 *           given to us from the ARCS firmware.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * $Id: memory.c,v 1.8 2000/01/17 23:32:46 ralf Exp $
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <linux/config.h>

#include <asm/sgialib.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/bootinfo.h>

#undef DEBUG

extern char _end;

struct linux_mdesc * __init prom_getmdesc(struct linux_mdesc *curr)
{
	return romvec->get_mdesc(curr);
}

#ifdef DEBUG /* convenient for debugging */
static char *arcs_mtypes[8] = {
	"Exception Block",
	"ARCS Romvec Page",
	"Free/Contig RAM",
	"Generic Free RAM",
	"Bad Memory",
	"Standlong Program Pages",
	"ARCS Temp Storage Area",
	"ARCS Permanent Storage Area"
};

static char *arc_mtypes[8] = {
	"Exception Block",
	"SystemParameterBlock",
	"FreeMemory",
	"Bad Memory",
	"LoadedProgram",
	"FirmwareTemporary",
	"FirmwarePermanent",
	"FreeContigiuous"
};
#define mtypes(a) (prom_flags & PROM_FLAG_ARCS) ? arcs_mtypes[a.arcs] : arc_mtypes[a.arc]
#endif

static struct prom_pmemblock prom_pblocks[PROM_MAX_PMEMBLOCKS];

#define MEMTYPE_DONTUSE   0
#define MEMTYPE_PROM      1
#define MEMTYPE_FREE      2

static inline int memtype_classify_arcs (union linux_memtypes type)
{
	switch (type.arcs) {
	case arcs_fcontig:
	case arcs_free:
		return MEMTYPE_FREE;
	case arcs_atmp:
		return MEMTYPE_PROM;
	case arcs_eblock:
	case arcs_rvpage:
	case arcs_bmem:
	case arcs_prog:
	case arcs_aperm:
		return MEMTYPE_DONTUSE;
	default:
		BUG();
	}
	while(1);				/* Nuke warning.  */
}

static inline int memtype_classify_arc (union linux_memtypes type)
{
	switch (type.arc) {
	case arc_free:
	case arc_fcontig:
		return MEMTYPE_FREE;
	case arc_atmp:
		return MEMTYPE_PROM;
	case arc_eblock:
	case arc_rvpage:
	case arc_bmem:
	case arc_prog:
	case arc_aperm:
		return MEMTYPE_DONTUSE;
	default:
		BUG();
	}
	while(1);				/* Nuke warning.  */
}

static int __init prom_memtype_classify (union linux_memtypes type)
{
	if (prom_flags & PROM_FLAG_ARCS)	/* SGI is ``different'' ...  */
		return memtype_classify_arc(type);

	return memtype_classify_arc(type);
}

static unsigned long __init find_max_low_pfn(void)
{
	struct prom_pmemblock *p, *highest;

	for (p = prom_pblocks, highest = 0; p->size != 0; p++) {
		if (!highest || p->base > highest->base)
			highest = p;
	}
#ifdef DEBUG
	prom_printf("find_max_low_pfn: mips_memory_upper = %08lx\n", highest);
#endif
	return (highest->base + highest->size) >> PAGE_SHIFT;
}

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

void __init prom_meminit(void)
{
	unsigned long start_pfn;
	struct linux_mdesc *p;
	int totram;
	int i = 0;

#ifdef DEBUG
	prom_printf("ARCS MEMORY DESCRIPTOR dump:\n");
	p = prom_getmdesc(PROM_NULL_MDESC);
	while(p) {
		prom_printf("[%d,%p]: base<%08lx> pages<%08lx> type<%s>\n",
			    i, p, p->base, p->pages, mtypes(p->type));
		p = prom_getmdesc(p);
		i++;
	}
#endif

	totram = 0;
	p = prom_getmdesc(PROM_NULL_MDESC);
	i = 0;
	while (p) {
		prom_pblocks[i].type = prom_memtype_classify(p->type);
		prom_pblocks[i].base = p->base << PAGE_SHIFT;
		prom_pblocks[i].size = p->pages << PAGE_SHIFT;

		switch (prom_pblocks[i].type) {
		case MEMTYPE_FREE:
			totram += prom_pblocks[i].size;
#ifdef DEBUG
			prom_printf("free_chunk[%d]: base=%08lx size=%d\n",
				    i, prom_pblocks[i].base,
				    prom_pblocks[i].size);
#endif
			i++;
			break;
		case MEMTYPE_PROM:
#ifdef DEBUG
			prom_printf("prom_chunk[%d]: base=%08lx size=%d\n",
				    i, prom_pblocks[i].base,
				    prom_pblocks[i].size);
#endif
			i++;
			break;
		default:
			break;
		}
		p = prom_getmdesc(p);
	}
	prom_pblocks[i].size = 0;

	/* Setup upper physical memory bound. */
	max_low_pfn = find_max_low_pfn();

	start_pfn = PFN_UP((unsigned long)&_end - PAGE_OFFSET);
	init_bootmem(start_pfn, max_low_pfn);

	for (i = 0; prom_pblocks[i].size; i++)
		if (prom_pblocks[i].type == MEMTYPE_FREE)
			free_bootmem(prom_pblocks[i].base, prom_pblocks[i].size);

	printk("PROMLIB: Total free ram %d bytes (%dK,%dMB)\n",
	       totram, (totram/1024), (totram/1024/1024));
}

void __init prom_free_prom_memory (void)
{
	struct prom_pmemblock *p;
	unsigned long freed = 0;
	unsigned long addr;

	for (p = prom_pblocks; p->size != 0; p++) {
		if (p->type != MEMTYPE_PROM)
			continue;

		addr = PAGE_OFFSET + p->base;
		while (addr < p->base + p->size) {
			ClearPageReserved(mem_map + MAP_NR(addr));
			set_page_count(mem_map + MAP_NR(addr), 1);
			free_page(addr);
			addr += PAGE_SIZE;
			freed += PAGE_SIZE;
		}
	}
	printk("Freeing prom memory: %ldk freed\n", freed >> 10);
}
