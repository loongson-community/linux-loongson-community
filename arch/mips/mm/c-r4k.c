/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r4xx0.c: R4000 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bcache.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>
#include <asm/war.h>

/* Primary cache parameters. */
static unsigned long icache_size, dcache_size; /* Size in bytes */
static unsigned long ic_lsize, dc_lsize;       /* LineSize in bytes */

/* Secondary cache (if present) parameters. */
static unsigned long scache_size, sc_lsize;	/* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

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

/*
 * On processors with QED R4600 style two set assosicative cache
 * this is the bit which selects the way in the cache for the
 * indexed cachops.
 */
#define icache_waybit (icache_size >> 1)
#define dcache_waybit (dcache_size >> 1)

static void r4k_blast_dcache_page(unsigned long addr)
{
	unsigned int prid = read_c0_prid() & 0xfff0;
	static void *l = &&init;

	goto *l;

dc_16:
	blast_dcache16_page(addr);
	return;

dc_32:
	blast_dcache32_page(addr);
	return;

dc_32_r4600:
	{
#ifdef R4600_V1_HIT_DCACHE_WAR
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("nop;nop;nop;nop");
#endif
	blast_dcache32_page(addr);
#ifdef R4600_V1_HIT_DCACHE_WAR
	local_irq_restore(flags);
#endif
	return;
	}

init:
	if (prid == 0x2010)			/* R4600 V1.7 */
		l = &&dc_32_r4600;
	else if (dc_lsize == 16)
		l = &&dc_16;
	else if (dc_lsize == 32)
		l = &&dc_32;
	goto *l;
}

static void r4k_blast_dcache_page_indexed(unsigned long addr)
{
	static void *l = &&init;

	goto *l;

dc_16:
	blast_dcache16_page_indexed(addr);
	return;

dc_32:
	blast_dcache32_page_indexed(addr);
	return;

init:
	if (dc_lsize == 16)
		l = &&dc_16;
	else if (dc_lsize == 32)
		l = &&dc_32;
	goto *l;
}

static void r4k_blast_dcache(void)
{
	static void *l = &&init;

	goto *l;

dc_16:
	blast_dcache16();
	return;

dc_32:
	blast_dcache32();
	return;

init:
	if (dc_lsize == 16)
		l = &&dc_16;
	else if (dc_lsize == 32)
		l = &&dc_32;
	goto *l;
}

static void r4k_blast_icache_page(unsigned long addr)
{
	static void *l = &&init;

	goto *l;

ic_16:
	blast_icache16_page(addr);
	return;

ic_32:
	blast_icache32_page(addr);
	return;

init:
	if (ic_lsize == 16)
		l = &&ic_16;
	else if (ic_lsize == 32)
		l = &&ic_32;
	goto *l;
}

static void r4k_blast_icache_page_indexed(unsigned long addr)
{
	static void *l = &&init;

	goto *l;

ic_16:
	blast_icache16_page_indexed(addr);
	return;

ic_32:
	blast_icache32_page_indexed(addr);
	return;

init:
	if (ic_lsize == 16)
		l = &&ic_16;
	else if (ic_lsize == 32)
		l = &&ic_32;
	goto *l;
}

static void r4k_blast_icache(void)
{
	static void *l = &&init;

	goto *l;

ic_16:
	blast_icache16();
	return;

ic_32:
	blast_icache32();
	return;

init:
	if (ic_lsize == 16)
		l = &&ic_16;
	else if (ic_lsize == 32)
		l = &&ic_32;
	goto *l;
}

static void r4k_blast_scache(void)
{
	static void *l = &&init;

	goto *l;

sc_16:
	blast_scache16();
	return;

sc_32:
	blast_scache32();
	return;

sc_64:
	blast_scache64();
	return;

sc_128:
	blast_scache128();
	return;

init:
	if (sc_lsize == 16)
		l = &&sc_16;
	else if (sc_lsize == 32)
		l = &&sc_32;
	else if (sc_lsize == 64)
		l = &&sc_64;
	else if (sc_lsize == 128)
		l = &&sc_128;
	goto *l;
}

static inline void r4k_flush_scache_all(void)
{
	r4k_blast_dcache();
	r4k_blast_icache();
	r4k_blast_scache();
}

static inline void r4k_flush_pcache_all(void)
{
	r4k_blast_dcache();
	r4k_blast_icache();
}

static void r4k_flush_cache_range(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	if (cpu_context(smp_processor_id(), vma->vm_mm) != 0) {
		r4k_blast_dcache();
		if (vma->vm_flags & VM_EXEC)
			r4k_blast_icache();
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void r4k_flush_scache_mm(struct mm_struct *mm)
{
	if (cpu_context(smp_processor_id(), mm) != 0) {
		r4k_flush_scache_all();
	}
}

static void r4k_flush_pcache_mm(struct mm_struct *mm)
{
	if (cpu_context(smp_processor_id(), mm) != 0) {
		r4k_flush_pcache_all();
	}
}

static void r4k_flush_cache_page(struct vm_area_struct *vma,
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
		r4k_blast_dcache_page(page);
		if (exec)
			r4k_blast_icache_page(page);

		return;
	}

	/*
	 * Do indexed flush, too much work to get the (possible) TLB refills
	 * to work correctly.
	 */
	page = (KSEG0 + (page & (dcache_size - 1)));
	r4k_blast_dcache_page_indexed(page);
	if (exec)
		r4k_blast_icache_page_indexed(page);
}

static void r4k_flush_cache_page_r4600(struct vm_area_struct *vma,
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
	if (!(pte_val(*ptep) & _PAGE_PRESENT))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if ((mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		r4k_blast_dcache_page(page);
		if (exec)
			r4k_blast_icache_page(page);

		return;
	}

	/*
	 * Do indexed flush, too much work to get the (possible)
	 * tlb refills to work correctly.
	 */
	page = KSEG0 + (page & (dcache_size - 1));
	r4k_blast_dcache_page_indexed(page);
	r4k_blast_dcache_page_indexed(page ^ dcache_waybit);

	if (exec) {
		r4k_blast_icache_page_indexed(page);
		r4k_blast_icache_page_indexed(page ^ icache_waybit);
	}
}

static void r4k_flush_data_cache_page(unsigned long addr)
{
	r4k_blast_dcache_page(addr);
}

static void r4k_flush_icache_range(unsigned long start, unsigned long end)
{
	unsigned long addr, aend;

	if (end - start > dcache_size)
		r4k_blast_dcache();
	else {
		addr = start & ~(dc_lsize - 1);
		aend = (end - 1) & ~(dc_lsize - 1);
		while (1) {
			/* Hit_Writeback_Inv_D */
			protected_writeback_dcache_line(addr);
			if (addr == aend)
				break;
			addr += dc_lsize;
		}
	}

	if (end - start > icache_size)
		r4k_blast_icache();
	else {
		addr = start & ~(dc_lsize - 1);
		aend = (end - 1) & ~(dc_lsize - 1);
		while (1) {
			/* Hit_Invalidate_I */
			protected_flush_icache_line(addr);
			if (addr == aend)
				break;
			addr += dc_lsize;
		}
	}
}

/*
 * Ok, this seriously sucks.  We use them to flush a user page but don't
 * know the virtual address, so we have to blast away the whole icache
 * which is significantly more expensive than the real thing.  Otoh we at
 * least know the kernel address of the page so we can flush it
 * selectivly.
 */
static void r4k_flush_icache_page(struct vm_area_struct *vma,
	struct page *page)
{
	if (vma->vm_flags & VM_EXEC) {
		unsigned long addr = (unsigned long) page_address(page);

		r4k_blast_dcache_page(addr);
		r4k_blast_icache();
	}
}

static void r4k_dma_cache_wback_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		r4k_flush_pcache_all();
	} else {
#ifdef R4600_V2_HIT_CACHEOP_WAR
		unsigned long flags;

		/* Workaround for R4600 bug.  See comment in <asm/war>. */
		local_irq_save(flags);
		*(volatile unsigned long *)KSEG1;
#endif

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
#ifdef R4600_V2_HIT_CACHEOP_WAR
		local_irq_restore(flags);
#endif
	}

	bc_wback_inv(addr, size);
}

static void r4k_dma_cache_wback_inv_sc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= scache_size) {
		r4k_flush_scache_all();
		return;
	}

	a = addr & ~(sc_lsize - 1);
	end = (addr + size - 1) & ~(sc_lsize - 1);
	while (1) {
		flush_scache_line(a);	/* Hit_Writeback_Inv_SD */
		if (a == end)
			break;
		a += sc_lsize;
	}
}

static void r4k_dma_cache_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		r4k_flush_pcache_all();
	} else {
#ifdef R4600_V2_HIT_CACHEOP_WAR
		unsigned long flags;

		/* Workaround for R4600 bug.  See comment in <asm/war>. */
		local_irq_save(flags);
		*(volatile unsigned long *)KSEG1;
#endif

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
			if (a == end)
				break;
			a += dc_lsize;
		}
#ifdef R4600_V2_HIT_CACHEOP_WAR
		local_irq_restore(flags);
#endif
	}

	bc_inv(addr, size);
}

static void r4k_dma_cache_inv_sc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= scache_size) {
		r4k_flush_scache_all();
		return;
	}

	a = addr & ~(sc_lsize - 1);
	end = (addr + size - 1) & ~(sc_lsize - 1);
	while (1) {
		flush_scache_line(a);	/* Hit_Writeback_Inv_SD */
		if (a == end)
			break;
		a += sc_lsize;
	}
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void r4k_flush_cache_sigtramp(unsigned long addr)
{
#ifdef R4600_V1_HIT_DCACHE_WAR
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("nop;nop;nop;nop");
#endif

	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));

#ifdef R4600_V1_HIT_DCACHE_WAR
	local_irq_restore(flags);
#endif
}

static void r4600v20k_flush_cache_sigtramp(unsigned long addr)
{
#ifdef R4600_V2_HIT_CACHEOP_WAR
	unsigned long flags;

	local_irq_save(flags);

	/* Clear internal cache refill buffer */
	*(volatile unsigned int *)KSEG1;
#endif

	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));

#ifdef R4600_V2_HIT_CACHEOP_WAR
	local_irq_restore(flags);
#endif
}

static void __init probe_icache(unsigned long config)
{
	switch (current_cpu_data.cputype) {
	case CPU_VR41XX:
	case CPU_VR4111:
	case CPU_VR4121:
	case CPU_VR4122:
	case CPU_VR4131:
	case CPU_VR4181:
	case CPU_VR4181A:
		icache_size = 1 << (10 + ((config >> 9) & 7));
		break;
	default:
		icache_size = 1 << (12 + ((config >> 9) & 7));
		break;
	}
	ic_lsize = 16 << ((config >> 5) & 1);

	/*
	 * Processor configuration sanity check for the R4000SC V2.2
	 * erratum #5.  With pagesizes larger than 32kb there is no possibility
	 * to get a VCE exception anymore so we don't care about this
	 * missconfiguration.  The case is rather theoretical anway; presumably
	 * no vendor is shipping his hardware in the "bad" configuration.
	 */
	if (PAGE_SIZE <= 0x8000 && read_c0_prid() == 0x0422 &&
	    (read_c0_config() & CONF_SC) && ic_lsize != 16)
		panic("Impropper processor configuration detected");

	printk("Primary instruction cache %ldK, linesize %ld bytes.\n",
	       icache_size >> 10, ic_lsize);
}

static void __init probe_dcache(unsigned long config)
{
	switch (current_cpu_data.cputype) {
	case CPU_VR41XX:
	case CPU_VR4111:
	case CPU_VR4121:
	case CPU_VR4122:
	case CPU_VR4131:
	case CPU_VR4181:
	case CPU_VR4181A:
		dcache_size = 1 << (10 + ((config >> 6) & 7));
		break;
	default:
		dcache_size = 1 << (12 + ((config >> 6) & 7));
		break;
	}
	dc_lsize = 16 << ((config >> 4) & 1);

	printk("Primary data cache %ldK, linesize %ld bytes.\n",
	       dcache_size >> 10, dc_lsize);
}

/*
 * If you even _breathe_ on this function, look at the gcc output and make sure
 * it does not pop things on and off the stack for the cache sizing loop that
 * executes in KSEG1 space or else you will crash and burn badly.  You have
 * been warned.
 */
static int __init probe_scache(unsigned long config)
{
	extern unsigned long stext;
	unsigned long flags, addr, begin, end, pow2;
	int tmp;

	if ((config >> 17) & 1)
		return 0;

	sc_lsize = 16 << ((config >> 22) & 3);

	begin = (unsigned long) &stext;
	begin &= ~((4 * 1024 * 1024) - 1);
	end = begin + (4 * 1024 * 1024);

	/*
	 * This is such a bitch, you'd think they would make it easy to do
	 * this.  Away you daemons of stupidity!
	 */
	local_irq_save(flags);

	/* Fill each size-multiple cache line with a valid tag. */
	pow2 = (64 * 1024);
	for (addr = begin; addr < end; addr = (begin + pow2)) {
		unsigned long *p = (unsigned long *) addr;
		__asm__ __volatile__("nop" : : "r" (*p)); /* whee... */
		pow2 <<= 1;
	}

	/* Load first line with zero (therefore invalid) tag. */
	write_c0_taglo(0);
	write_c0_taghi(0);
	__asm__ __volatile__("nop; nop; nop; nop;"); /* avoid the hazard */
	cache_op(Index_Store_Tag_I, begin);
	cache_op(Index_Store_Tag_D, begin);
	cache_op(Index_Store_Tag_SD, begin);

	/* Now search for the wrap around point. */
	pow2 = (128 * 1024);
	tmp = 0;
	for (addr = begin + (128 * 1024); addr < end; addr = begin + pow2) {
		cache_op(Index_Load_Tag_SD, addr);
		__asm__ __volatile__("nop; nop; nop; nop;"); /* hazard... */
		if (!read_c0_taglo())
			break;
		pow2 <<= 1;
	}
	local_irq_restore(flags);
	addr -= begin;
	printk("Secondary cache sized at %ldK, linesize %ld bytes.\n",
	       addr >> 10, sc_lsize);
	scache_size = addr;

	return 1;
}

static void __init setup_noscache_funcs(void)
{
	unsigned int prid;

	switch (dc_lsize) {
	case 16:
		_clear_page = r4k_clear_page_d16;
		_copy_page = r4k_copy_page_d16;

		break;
	case 32:
		prid = read_c0_prid() & 0xfff0;
		if (prid == 0x2010) {			/* R4600 V1.7 */
			_clear_page = r4k_clear_page_r4600_v1;
			_copy_page = r4k_copy_page_r4600_v1;
		} else if (prid == 0x2020) {		/* R4600 V2.0 */
			_clear_page = r4k_clear_page_r4600_v2;
			_copy_page = r4k_copy_page_r4600_v2;
		} else {
			_clear_page = r4k_clear_page_d32;
			_copy_page = r4k_copy_page_d32;
		}
		break;
	}
	flush_cache_all = r4k_flush_pcache_all;
	__flush_cache_all = r4k_flush_pcache_all;
	flush_cache_mm = r4k_flush_pcache_mm;
	flush_cache_page = r4k_flush_cache_page;
	flush_icache_page = r4k_flush_icache_page;
	flush_cache_range = r4k_flush_cache_range;

	_dma_cache_wback_inv = r4k_dma_cache_wback_inv_pc;
	_dma_cache_wback = r4k_dma_cache_wback_inv_pc;
	_dma_cache_inv = r4k_dma_cache_inv_pc;
}

static void __init setup_scache_funcs(void)
{
	if (dc_lsize > sc_lsize)
		panic("Invalid primary cache configuration detected");

	switch (sc_lsize) {
	case 16:
		_clear_page = r4k_clear_page_s16;
		_copy_page = r4k_copy_page_s16;
		break;
	case 32:
		_clear_page = r4k_clear_page_s32;
		_copy_page = r4k_copy_page_s32;
		break;
	case 64:
		_clear_page = r4k_clear_page_s64;
		_copy_page = r4k_copy_page_s64;
		break;
	case 128:
		_clear_page = r4k_clear_page_s128;
		_copy_page = r4k_copy_page_s128;
		break;
	}

	flush_cache_all = r4k_flush_pcache_all;
	__flush_cache_all = r4k_flush_scache_all;
	flush_cache_mm = r4k_flush_scache_mm;
	flush_cache_range = r4k_flush_cache_range;
	flush_cache_page = r4k_flush_cache_page;
	flush_icache_page = r4k_flush_icache_page;

	_dma_cache_wback_inv = r4k_dma_cache_wback_inv_sc;
	_dma_cache_wback = r4k_dma_cache_wback_inv_sc;
	_dma_cache_inv = r4k_dma_cache_inv_sc;
}

typedef int (*probe_func_t)(unsigned long);
extern int r5k_sc_init(void);

static inline void __init setup_scache(unsigned int config)
{
	probe_func_t probe_scache_kseg1;
	int sc_present = 0;

	/* Maybe the cpu knows about a l2 cache? */
	probe_scache_kseg1 = (probe_func_t) (KSEG1ADDR(&probe_scache));
	sc_present = probe_scache_kseg1(config);

	if (!sc_present) {
		setup_noscache_funcs();
		return;
	}

	switch(current_cpu_data.cputype) {
	case CPU_R5000:
	case CPU_NEVADA:
			setup_noscache_funcs();
#ifdef CONFIG_R5000_CPU_SCACHE
			r5k_sc_init();
#endif
			break;
	default:
			setup_scache_funcs();
	}
}

void __init ld_mmu_r4xx0(void)
{
	unsigned long config = read_c0_config();
	extern char except_vec2_generic;
	unsigned int sets;

	/* Default cache error handler for R4000 family */

	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_generic, 0x80);

	change_c0_config(CONF_CM_CMASK | CONF_CU, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);
	setup_scache(config);

	switch(current_cpu_data.cputype) {
	case CPU_R4600:			/* QED style two way caches? */
	case CPU_R4700:
	case CPU_R5000:
	case CPU_NEVADA:
		flush_cache_page = r4k_flush_cache_page_r4600;
		sets = 1;
		break;

	default:
		sets = 0;
		break;
	}

	shm_align_mask = max_t(unsigned long,
	                       (dcache_size >> sets) - 1, PAGE_SIZE - 1);

	flush_cache_sigtramp = r4k_flush_cache_sigtramp;
	if ((read_c0_prid() & 0xfff0) == 0x2020) {
		flush_cache_sigtramp = r4600v20k_flush_cache_sigtramp;
	}
	flush_data_cache_page = r4k_flush_data_cache_page;
	flush_icache_range = r4k_flush_icache_range;	/* Ouch */

	__flush_cache_all();
}
