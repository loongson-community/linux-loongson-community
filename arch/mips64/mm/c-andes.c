/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 1999 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000 Kanoj Sarcar (kanoj@sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/r10kcache.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

extern void andes_clear_page(void * page);
extern void andes_copy_page(void * to, void * from);

/*
 * Cache operations.  These are only used with the virtual memory system,
 * not for non-coherent I/O so it's ok to ignore the secondary caches.
 */
static void andes_flush_cache_all(void)
{
}

static void andes___flush_cache_all(void)
{
	if (sc_lsize() == 64)
		blast_scache64();
	else
		blast_scache128();
}

static void andes_flush_cache_mm(struct mm_struct *mm)
{
}

static void andes_flush_cache_range(struct mm_struct *mm, unsigned long start,
        unsigned long end)
{
}

static void andes_flush_cache_page(struct vm_area_struct *vma,
        unsigned long page)
{
}

static void andes_flush_dcache_page(struct page * page)
{
}

static void andes_flush_icache_range(unsigned long start, unsigned long end)
{
	blast_dcache32(); blast_icache64();
}

/*
 * Tricky ...  Because we don't know the virtual address we've got the choice
 * of either invalidating the entire primary and secondary caches or
 * invalidating the secondary caches also.  On the R10000 invalidating the
 * secondary cache will result in any entries in the primary caches also
 * getting invalidated.
 */
void andes_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	unsigned long addr;

	if (!((vma)->vm_flags & VM_EXEC))
		return;

	addr = (unsigned long) phys_to_virt(page_to_phys(page));
	if (sc_lsize() == 64)
		blast_scache64_page(addr);
	else
		blast_scache128_page(addr);
}

static void andes_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte)
{
}

void __init ld_mmu_andes(void)
{
	/* Default cache error handler for SB1 */
	extern char except_vec2_generic;

	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_generic, 0x80);

	printk("Primary instruction cache %dkb, linesize %d bytes\n",
	       icache_size >> 10, ic_lsize);
	printk("Primary data cache %dkb, linesize %d bytes\n",
	       dcache_size >> 10, dc_lsize);
	printk("Secondary cache sized at %ldK, linesize %ld\n",
	       scache_size() >> 10, sc_lsize());

	_clear_page = andes_clear_page;
	_copy_page = andes_copy_page;

	_flush_cache_all = andes_flush_cache_all;
	___flush_cache_all = andes___flush_cache_all;
	_flush_cache_mm = andes_flush_cache_mm;
	_flush_cache_range = andes_flush_cache_range;
	_flush_cache_page = andes_flush_cache_page;
	_flush_dcache_page = andes_flush_dcache_page;
	_flush_icache_range = andes_flush_icache_range;
	_flush_icache_page = andes_flush_icache_page;

	_flush_cache_sigtramp = andes_flush_cache_sigtramp;

	__flush_cache_all();
}
