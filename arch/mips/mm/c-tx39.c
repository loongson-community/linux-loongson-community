/*
 * r2300.c: R2000 and R3000 specific mmu/cache code.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * with a lot of changes to make this thing work for R3000s
 * Tx39XX R4k style caches added. HK
 * Copyright (C) 1998, 1999, 2000 Harald Koerfgen
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/cacheops.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/isadep.h>
#include <asm/io.h>
#include <asm/wbflush.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>

/* For R3000 cores with R4000 style caches */
static unsigned long icache_size, dcache_size;		/* Size in bytes */
extern long scache_size;

#define icache_lsize 16			/* for all current cpu variants  */
#define dcache_lsize 4			/* for all current cpu variants  */

#include <asm/r4kcache.h>

static void tx39_flush_page_to_ram(struct page * page)
{
}

static void tx39_flush_icache_all(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + icache_size);
	unsigned long dummy = 0;

	/* disable icache and stop streaming */
	__asm__ __volatile__(
	".set\tnoreorder\n\t"
	"mfc0\t%0, $3\n\t"
	"xori\t%0, 32\n\t"
	"mtc0\t%0, $3\n\t"
	"j\t1f\n\t"
	"nop\n\t"
	"1:\t.set\treorder\n\t"
	: "+r"(dummy));

	/* invalidate icache */
	while (start < end) {
		cache16_unroll32(start, Index_Invalidate_I);
		start += 0x200;
	}

	/* enable icache */
	__asm__ __volatile__(
	".set\tnoreorder\n\t"
	"mfc0\t%0, $3\n\t"
	"xori\t%0, 32\n\t"
	"mtc0\t%0, $3\n\t"
	".set\treorder\n\t"
	: "+r"(dummy));
}

static __init void tx39_probe_cache(void)
{
	unsigned long config;

	config = read_32bit_cp0_register(CP0_CONF);

	icache_size = 1 << (10 + ((config >> 19) & 3));
	dcache_size = 1 << (10 + ((config >> 16) & 3));
}

void __init ld_mmu_tx39(void)
{
	unsigned long config;

	printk("CPU revision is: %08x\n", read_32bit_cp0_register(CP0_PRID));

	_clear_page = r3k_clear_page;
	_copy_page = r3k_copy_page;

	config = read_32bit_cp0_register(CP0_CONF);
	config &= ~TX39_CONF_WBON;
	write_32bit_cp0_register(CP0_CONF, config);

	tx39_probe_cache();

	_flush_cache_all	= tx39_flush_icache_all;
	___flush_cache_all	= tx39_flush_icache_all;
	_flush_cache_mm		= (void *) tx39_flush_icache_all;
	_flush_cache_range	= (void *) tx39_flush_icache_all;
	_flush_cache_page	= (void *) tx39_flush_icache_all;
	_flush_cache_sigtramp	= (void *) tx39_flush_icache_all;
	_flush_page_to_ram	= tx39_flush_page_to_ram;
	_flush_icache_page	= (void *) tx39_flush_icache_all;
	_flush_icache_range	= (void *) tx39_flush_icache_all;

	/* This seems to be wrong for a R4k-style caches -- Ralf  */
	// _dma_cache_wback_inv	= r3k_dma_cache_wback_inv;

	printk("Primary instruction cache %dkb, linesize %d bytes\n",
		(int) (icache_size >> 10), (int) icache_lsize);
	printk("Primary data cache %dkb, linesize %d bytes\n",
		(int) (dcache_size >> 10), (int) dcache_lsize);
}
