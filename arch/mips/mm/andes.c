/*
 * andes.c: MMU and cache operations for the R10000 (ANDES).
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

/* Cache operations. XXX Write these dave... */
static inline void andes_flush_cache_all(void)
{
	/* XXX */
}

static void andes_flush_cache_mm(struct mm_struct *mm)
{
	/* XXX */
}

static void andes_flush_cache_range(struct mm_struct *mm,
				    unsigned long start,
				    unsigned long end)
{
	/* XXX */
}

static void andes_flush_cache_page(struct vm_area_struct *vma,
				   unsigned long page)
{
	/* XXX */
}

static void andes_flush_page_to_ram(struct page * page)
{
	/* XXX */
}

static void __andes_flush_icache_range(unsigned long start, unsigned long end)
{
	/* XXX */
}

static void andes_flush_icache_page(struct vm_area_struct *vma,
                                    struct page *page)
{
	/* XXX */
}

static void andes_flush_cache_sigtramp(unsigned long page)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

/* TLB operations. XXX Write these dave... */
void local_flush_tlb_all(void)
{
	/* XXX */
}

void local_flush_tlb_mm(struct mm_struct *mm)
{
	/* XXX */
}

void local_flush_tlb_range(struct mm_struct *mm, unsigned long start,
				  unsigned long end)
{
	/* XXX */
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	/* XXX */
}

void pgd_init(unsigned long page)
{
}

void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
				  unsigned long entryhi, unsigned long pagemask)
{
        /* XXX */
}

void __init ld_mmu_andes(void)
{
	_clear_page = andes_clear_page;
	_copy_page = andes_copy_page;

	_flush_cache_all = andes_flush_cache_all;
	___flush_cache_all = andes_flush_cache_all;
	_flush_cache_mm = andes_flush_cache_mm;
	_flush_cache_range = andes_flush_cache_range;
	_flush_cache_page = andes_flush_cache_page;
	_flush_cache_sigtramp = andes_flush_cache_sigtramp;
	_flush_page_to_ram = andes_flush_page_to_ram;
	_flush_icache_page = andes_flush_icache_page;
	_flush_icache_range = andes_flush_icache_range;

	write_32bit_cp0_register(CP0_FRAMEMASK, 0);

	flush_cache_all();
	local_flush_tlb_all();

	/*
	 * The R10k might even work for Linux/MIPS - but we're paranoid
	 * and refuse to run until this is tested on real silicon
	 */
	panic("CPU too expensive - making holiday in the ANDES!");
}
