/*
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000, 2002 MIPS Technologies, Inc.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/bcache.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

extern void mips3264_clear_page_dc(void *page);
extern void mips3264_copy_page_dc(void *to, void *from);

/* Primary cache parameters. */
static unsigned long icache_size, dcache_size;	/* Size in bytes */
static unsigned long ic_lsize, dc_lsize;	/* LineSize in bytes */

/* Secondary cache (if present) parameters. */
unsigned int scache_size, sc_lsize;		/* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/mips3264-cache.h>

/*
 * Dummy cache handling routines for machines without boardcaches
 */
static void no_sc_noop(void) {}

static struct bcache_ops no_sc_ops = {
	.bc_enable = (void *)no_sc_noop,
	.bc_disable = (void *)no_sc_noop,
	.bc_wback_inv = (void *)no_sc_noop,
	.bc_inv = (void *)no_sc_noop
};

struct bcache_ops *bcops = &no_sc_ops;

static inline void mips3264_flush_cache_all_pc(void)
{
	if (!cpu_has_dc_aliases)
		return;

	blast_dcache();
	blast_icache();
}

static inline void mips3264___flush_cache_all_pc(void)
{
	blast_dcache();
	blast_icache();
}

static void mips3264_flush_cache_range_pc(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	if (!cpu_has_dc_aliases)
		return;

	if (cpu_context(smp_processor_id(), mm) != 0) {
		blast_dcache();
		if (vma->vm_flags & VM_EXEC)
			blast_icache();
	}
}

static void mips3264_flush_cache_mm_pc(struct mm_struct *mm)
{
	if (!cpu_has_dc_aliases)
		return;

	if (cpu_context(smp_processor_id(), mm) != 0)
		mips3264_flush_cache_all_pc();
}

static void mips3264_flush_cache_page_pc(struct vm_area_struct *vma,
				    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int exec;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if (cpu_context(smp_processor_id(), mm) == 0)
		return;

	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if (!(pte_val(*ptep) & _PAGE_PRESENT))
		return;

	/*
	 * Doing flushes for another ASID than the current one is too difficult
	 * since MIPS32 and MIPS64 caches do a TLB translation for every cache
	 * flush operation.  So we do indexed flushes in that case, which
	 * doesn't overly flush the cache too much.
	 */
	exec = vma->vm_flags & VM_EXEC;
	if ((mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		if (cpu_has_dc_aliases || exec)
			blast_dcache_page(page);
		if (exec)
			blast_icache_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = KSEG0 + (page & (dcache_size - 1));
		if (cpu_has_dc_aliases || exec)
			blast_dcache_page_indexed(page);
		if (exec)
			blast_icache_page_indexed(page);
	}
}

static void mips3264_flush_data_cache_page(unsigned long addr)
{
	blast_dcache_page(addr);
}

static void
mips3264_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

static void
mips3264_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	unsigned long addr;

	/*
	 * If there's no context yet, or the page isn't executable, no icache 
	 * flush is needed.
	 */
	if (!(vma->vm_flags & VM_EXEC))
		return;

	/*
	 * We're not sure of the virtual address(es) involved here, so
	 * conservatively flush the entire caches.
	 */
	addr = (unsigned long) page_address(page);
	blast_dcache_page(addr);
	blast_icache();
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
mips3264_dma_cache_wback_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		blast_dcache();
	} else {
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
	}
	bc_wback_inv(addr, size);
}

static void mips3264_dma_cache_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		blast_dcache();
	} else {
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			invalidate_dcache_line(a); /* Hit_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
	}

	bc_inv(addr, size);
}

static void
mips3264_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("mips3264_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void mips3264_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

static void mips3264_flush_icache_all(void)
{
	if (cpu_has_vtag_icache)
		blast_icache();
}

static char *way_string[] = { NULL, "direct mapped", "2-way", "3-way", "4-way",
	"5-way", "6-way", "7-way", "8-way"
};

/* Detect and size the various caches. */
static void __init probe_icache(unsigned long config)
{
        unsigned long config1;
	unsigned int lsize;

	current_cpu_data.icache.flags = 0;
	config1 = read_c0_config1();

	if ((lsize = ((config1 >> 19) & 7)))
		current_cpu_data.icache.linesz = 2 << lsize;
	else
		current_cpu_data.icache.linesz = lsize;
	current_cpu_data.icache.sets = 64 << ((config1 >> 22) & 7);
	current_cpu_data.icache.ways = 1 + ((config1 >> 16) & 7);

	ic_lsize = current_cpu_data.icache.linesz;
	icache_size = current_cpu_data.icache.sets *
	              current_cpu_data.icache.ways * ic_lsize;

	if ((config & 0x8) || (current_cpu_data.cputype == CPU_20KC)) {
		/* 
		 * The CPU has a virtually tagged I-cache.
		 * Some older 20Kc chips doesn't have the 'VI' bit in
		 * the config register, so we also check for 20Kc.
		 */
		current_cpu_data.icache.flags |= MIPS_CACHE_VTAG;
	}

	printk("Primary instruction cache %ldkb %s, linesize %ld bytes\n",
	       icache_size >> 10, way_string[current_cpu_data.icache.ways],
	       ic_lsize);
}

static void __init probe_dcache(unsigned long config)
{
	unsigned long config1;
	unsigned int lsize;

	current_cpu_data.dcache.flags = 0;
	config1 = read_c0_config1();

	if ((lsize = ((config1 >> 10) & 7)))
	        current_cpu_data.dcache.linesz = 2 << lsize;
	else
	        current_cpu_data.dcache.linesz= lsize;
	current_cpu_data.dcache.sets = 64 << ((config1 >> 13) & 7);
	current_cpu_data.dcache.ways = 1 + ((config1 >> 7) & 7);

	dc_lsize = current_cpu_data.dcache.linesz;
	dcache_size = current_cpu_data.dcache.sets *
	              current_cpu_data.dcache.ways * dc_lsize;

	printk("Primary data cache %ldkb %s, linesize %ld bytes\n",
	       dcache_size >> 10, way_string[current_cpu_data.dcache.ways],
	       dc_lsize);
}

void __init ld_mmu_mips3264(void)
{
	unsigned long config = read_c0_config();
	extern char except_vec2_generic;

	/* Default cache error handler for MIPS32 and MIPS64 */
	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_generic, 0x80);

	change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);

	if (!(current_cpu_data.scache.flags & MIPS_CACHE_NOT_PRESENT))
		panic("Dunno how to handle MIPS32 with second level cache");

	if (current_cpu_data.dcache.sets * current_cpu_data.dcache.ways >
	    PAGE_SIZE)
		current_cpu_data.dcache.flags |= MIPS_CACHE_ALIASES;

	/*
	 * XXX Some MIPS32 and MIPS64 processors have physically indexed caches.
	 * This code supports virtually indexed processors and will be
	 * unnecessarily unefficient on physically indexed processors.
	 */
	shm_align_mask = max_t(unsigned long, current_cpu_data.dcache.sets * dc_lsize,
	                       PAGE_SIZE) - 1;

	_clear_page		= mips3264_clear_page_dc;
	_copy_page		= mips3264_copy_page_dc;

	_flush_cache_all	= mips3264_flush_cache_all_pc;
	___flush_cache_all	= mips3264___flush_cache_all_pc;
	_flush_cache_mm		= mips3264_flush_cache_mm_pc;
	_flush_cache_range	= mips3264_flush_cache_range_pc;
	_flush_cache_page	= mips3264_flush_cache_page_pc;
	_flush_icache_range	= mips3264_flush_icache_range;	/* Ouch */
	_flush_icache_page	= mips3264_flush_icache_page;

	_flush_cache_sigtramp	= mips3264_flush_cache_sigtramp;
	_flush_icache_all	= mips3264_flush_icache_all;
	_flush_data_cache_page	= mips3264_flush_data_cache_page;

	_dma_cache_wback_inv	= mips3264_dma_cache_wback_inv_pc;
	_dma_cache_wback	= mips3264_dma_cache_wback;
	_dma_cache_inv		= mips3264_dma_cache_inv_pc;

	__flush_cache_all();
}
