/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r49xx.c: TX49 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998, 1999, 2000 Ralf Baechle ralf@gnu.org
 *
 * Modified for R4300/TX49xx (Jun/2001)
 * Copyright (C) 1999-2001 Toshiba Corporation
 *
 * To do:
 *
 *  - this code is a overbloated pig
 *  - many of the bug workarounds are not efficient at all, but at
 *    least they are functional ...
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
#define ic_lsize	mips_cpu.icache.linesz
#define dc_lsize	mips_cpu.dcache.linesz
static unsigned long scache_size;

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

static inline void tx49_blast_dcache_page(unsigned long addr)
{
	if (dc_lsize == 16)
		blast_dcache16_page(addr);
	else
		blast_dcache32_page(addr);
}

static inline void tx49_blast_dcache_page_indexed(unsigned long addr)
{
	if (dc_lsize == 16)
		blast_dcache16_page_indexed_wayLSB(addr);
	else
		blast_dcache32_page_indexed_wayLSB(addr);
}

static inline void tx49_blast_dcache(void)
{
	if (dc_lsize == 16)
		blast_dcache16_wayLSB();
	else
		blast_dcache32_wayLSB();
}

static inline void tx49_blast_icache_page(unsigned long addr)
{
	blast_icache32_page(addr);
}

/* TX49 does can not flush the line contains the CACHE insn itself... */
static inline void tx49_blast_icache_page_indexed(unsigned long addr)
{
	unsigned long flags, config;
	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_config();
	write_c0_config(config | TX49_CONF_IC);
	blast_icache32_page_indexed_wayLSB(addr);
	write_c0_config(config);
	local_irq_restore(flags);
}

static inline void tx49_blast_icache(void)
{
	unsigned long flags, config;
	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_config();
	write_c0_config(config | TX49_CONF_IC);
	blast_icache32_wayLSB();
	write_c0_config(config);
	local_irq_restore(flags);
}

static inline void tx49_flush_cache_all(void)
{
	tx49_blast_dcache();
	tx49_blast_icache();
}

static void tx49_flush_cache_range(struct vm_area_struct *vma,
				   unsigned long start,
				   unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (cpu_context(smp_processor_id(), mm) != 0) {
		tx49_blast_dcache();
		tx49_blast_icache();
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void tx49_flush_cache_mm(struct mm_struct *mm)
{
	if (cpu_context(smp_processor_id(), mm) != 0) {
		tx49_flush_cache_all();
	}
}

static void tx49_flush_cache_page(struct vm_area_struct *vma,
				  unsigned long page)
{
	int exec = vma->vm_flags & VM_EXEC;
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

	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if (!(pte_val(*ptep) & _PAGE_VALID))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if (mm == current->active_mm) {
		tx49_blast_dcache_page(page);
		if (exec)
			tx49_blast_icache_page(page);

		return;
	}

	/*
	 * Do indexed flush, too much work to get the (possible) TLB refills
	 * to work correctly.
	 */
	page = (KSEG0 + (page & (dcache_size - 1)));
	tx49_blast_dcache_page_indexed(page);
	if (exec)
		tx49_blast_icache_page_indexed(page);
}

static void tx49_flush_data_cache_page(unsigned long addr)
{
	tx49_blast_dcache_page(addr);
}

static void
tx49_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

/*
 * Ok, this seriously sucks.  We use them to flush a user page but don't
 * know the virtual address, so we have to blast away the whole icache
 * which is significantly more expensive than the real thing.  Otoh we at
 * least know the kernel address of the page so we can flush it
 * selectivly.
 */
static void
tx49_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	if (vma->vm_flags & VM_EXEC) {
		unsigned long addr = (unsigned long) page_address(page);

		tx49_blast_dcache_page(addr);
		tx49_blast_icache();
	}
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
tx49_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		tx49_blast_dcache();
	} else {
		int lsize = dc_lsize;
		a = addr & ~(lsize - 1);
		end = (addr + size - 1) & ~(lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += lsize;
		}
	}
}

static void
tx49_dma_cache_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		tx49_blast_dcache();
	} else {
		int lsize = dc_lsize;
		a = addr & ~(lsize - 1);
		end = (addr + size - 1) & ~(lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += lsize;
		}
	}
}

static void
tx49_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("tx49_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void tx49_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

/* Detect and size the various r4k caches. */
static void __init probe_icache(unsigned long config)
{
	icache_size = 1 << (12 + ((config >> 9) & 7));
	ic_lsize = 16 << ((config >> 5) & 1);
	mips_cpu.icache.ways = 1;
	mips_cpu.icache.sets = icache_size / mips_cpu.icache.linesz;

	printk("Primary instruction cache %dkb, linesize %d bytes.\n",
	       icache_size >> 10, ic_lsize);
}

static void __init probe_dcache(unsigned long config)
{
	dcache_size = 1 << (12 + ((config >> 6) & 7));
	dc_lsize = 16 << ((config >> 4) & 1);
	mips_cpu.dcache.ways = 1;
	mips_cpu.dcache.sets = dcache_size / mips_cpu.dcache.linesz;

	printk("Primary data cache %dkb, linesize %d bytes.\n",
	       dcache_size >> 10, dc_lsize);
}

int mips_configk0 __initdata = -1;	/* board-specific setup routine can override this */
void __init ld_mmu_tx49(void)
{
	unsigned long config = read_c0_config();

	/* Default cache error handler for SB1 */
	extern char except_vec2_generic;

	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_generic, 0x80);

	if (mips_configk0 != -1)
		change_c0_config(CONF_CM_CMASK, mips_configk0);
	else
		change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);

	switch (dc_lsize) {
	case 16:
		_clear_page = r4k_clear_page_d16;
		_copy_page = r4k_copy_page_d16;
		break;
	case 32:
		_clear_page = r4k_clear_page_d32;
		_copy_page = r4k_copy_page_d32;
		break;
	}
	flush_cache_all = tx49_flush_cache_all;
	__flush_cache_all = _flush_cache_all;
	flush_cache_mm = tx49_flush_cache_mm;
	flush_cache_range = tx49_flush_cache_range;
	flush_cache_page = tx49_flush_cache_page;
	flush_icache_range = tx49_flush_icache_range;	/* Ouch */
	flush_icache_page = tx49_flush_icache_page;

	flush_cache_sigtramp = tx49_flush_cache_sigtramp;
	flush_data_cache_page = tx49_flush_data_cache_page;

	_dma_cache_wback_inv = tx49_dma_cache_wback_inv;
	_dma_cache_wback = tx49_dma_cache_wback;
	_dma_cache_inv = tx49_dma_cache_inv;

	__flush_cache_all();
}
