/*
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
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
 *
 * MIPS32 CPU variant specific MMU/Cache routines.
 */
#include <linux/config.h>
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

/* Primary cache parameters. */
int icache_size, dcache_size; 			/* Size in bytes */
int ic_lsize, dc_lsize;				/* LineSize in bytes */

/* Secondary cache (if present) parameters. */
unsigned int scache_size, sc_lsize;		/* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/mips32_cache.h>

#undef DEBUG_CACHE

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

static inline void mips32_flush_cache_all_pc(void)
{
	blast_dcache();
	blast_icache();
}

static void mips32_flush_cache_range_pc(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (cpu_context(smp_processor_id(), mm) != 0) {
		blast_dcache();
		if (vma->vm_flags & VM_EXEC)
			blast_icache();
	}
}

static void mips32_flush_cache_mm_pc(struct mm_struct *mm)
{
	if (cpu_context(smp_processor_id(), mm) != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", cpu_context(smp_processor_id(), mm));
#endif
		mips32_flush_cache_all_pc();
	}
}

static void mips32_flush_cache_page_pc(struct vm_area_struct *vma,
				    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if (cpu_context(smp_processor_id(), mm) == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", cpu_context(smp_processor_id(), mm), page);
#endif
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
	 * Doing flushes for another ASID than the current one is
	 * too difficult since Mips32 caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if ((mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache_page_indexed(page);
	}
}

static void mips32_flush_data_cache_page(unsigned long addr)
{
	blast_dcache_page(addr);
}

static void
mips32_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

static void
mips32_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
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
	flush_cache_all();
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
mips32_dma_cache_wback_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned int flags;

	if (size >= dcache_size) {
		 blast_dcache();
	} else {
	        local_irq_save(flags);
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}
	bc_wback_inv(addr, size);
}

static void
mips32_dma_cache_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned int flags;

	if (size >= dcache_size) {
		blast_dcache();
	} else {
	        local_irq_save(flags);
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			invalidate_dcache_line(a); /* Hit_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}

	bc_inv(addr, size);
}

static void
mips32_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("mips32_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void mips32_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

static void mips32_flush_icache_all(void)
{
	if (mips_cpu.icache.flags | MIPS_CACHE_VTAG_CACHE)
		blast_icache();
}

/* Detect and size the various caches. */
static void __init probe_icache(unsigned long config)
{
        unsigned long config1;
	unsigned int lsize;

	mips_cpu.icache.flags = 0;
	config1 = read_c0_config1();

	if ((lsize = ((config1 >> 19) & 7)))
		mips_cpu.icache.linesz = 2 << lsize;
	else
	       mips_cpu.icache.linesz = lsize;
	mips_cpu.icache.sets = 64 << ((config1 >> 22) & 7);
	mips_cpu.icache.ways = 1 + ((config1 >> 16) & 7);

	ic_lsize = mips_cpu.icache.linesz;
	icache_size = mips_cpu.icache.sets * mips_cpu.icache.ways * ic_lsize;

	if ((config & 0x8) || (mips_cpu.cputype == CPU_20KC)) {
	       /* 
		* The CPU has a virtually tagged I-cache.
		* Some older 20Kc chips doesn't have the 'VI' bit in
		* the config register, so we also check for 20Kc.
		*/
	       mips_cpu.icache.flags = MIPS_CACHE_VTAG_CACHE;
	       printk("Virtually tagged I-cache detected\n");
	}

	printk("Primary instruction cache %dkb, linesize %d bytes (%d ways)\n",
	       icache_size >> 10, ic_lsize, mips_cpu.icache.ways);
}

static void __init probe_dcache(unsigned long config)
{
        unsigned long config1;
	unsigned int lsize;

	mips_cpu.dcache.flags = 0;
	config1 = read_c0_config1();

	if ((lsize = ((config1 >> 10) & 7)))
	        mips_cpu.dcache.linesz = 2 << lsize;
	else
	        mips_cpu.dcache.linesz= lsize;
	mips_cpu.dcache.sets = 64 << ((config1 >> 13) & 7);
	mips_cpu.dcache.ways = 1 + ((config1 >> 7) & 7);

	dc_lsize = mips_cpu.dcache.linesz;
	dcache_size = mips_cpu.dcache.sets * mips_cpu.dcache.ways * dc_lsize;

	printk("Primary data cache %dkb, linesize %d bytes (%d ways)\n",
	       dcache_size >> 10, dc_lsize, mips_cpu.dcache.ways);
}

void __init ld_mmu_mips32(void)
{
	unsigned long config = read_c0_config();
	extern char except_vec2_generic;

	/* Default cache error handler for MIPS32 */
	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_generic, 0x80);

	change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);

	if (!(mips_cpu.scache.flags & MIPS_CACHE_NOT_PRESENT))
		panic("Dunno how to handle MIPS32 with second level cache");

	/*
	 * XXX Some MIPS32 processors have physically indexed caches.  This
	 * code supports virtually indexed processors and will be unnecessarily
	 * unefficient on physically indexed processors.
	 */
	shm_align_mask = max_t(unsigned long, mips_cpu.dcache.sets * dc_lsize,
	                       PAGE_SIZE) - 1;

	_clear_page		= (void *)mips32_clear_page_dc;
	_copy_page		= (void *)mips32_copy_page_dc;

	flush_cache_all		= mips32_flush_cache_all_pc;
	__flush_cache_all	= mips32_flush_cache_all_pc;
	flush_cache_mm		= mips32_flush_cache_mm_pc;
	flush_cache_range	= mips32_flush_cache_range_pc;
	flush_cache_page	= mips32_flush_cache_page_pc;
	flush_icache_range	= mips32_flush_icache_range;	/* Ouch */
	flush_icache_page	= mips32_flush_icache_page;

	flush_cache_sigtramp	= mips32_flush_cache_sigtramp;
	flush_data_cache_page	= mips32_flush_data_cache_page;
	flush_icache_all	= mips32_flush_icache_all;

	_dma_cache_wback_inv	= mips32_dma_cache_wback_inv_pc;
	_dma_cache_wback	= mips32_dma_cache_wback;
	_dma_cache_inv		= mips32_dma_cache_inv_pc;

	__flush_cache_all();
}
