/*
 * Cache maintenance for R4000/R4400/R4600 CPUs.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright 1996 by Ralf Baechle
 * FIXME: Support for SC/MC version is missing.
 */
#include <linux/kernel.h>
#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/system.h>

#define STR(x)  __STR(x)
#define __STR(x)  #x

unsigned long page_colour_mask;

/*
 * Size of the caches
 * Line size of the caches
 */
unsigned int dcache_size, icache_size;
unsigned int dcache_line_size, icache_line_size;
unsigned long dcache_line_mask, icache_line_mask;

/*
 * Profiling counter
 */
extern unsigned int dflushes;
extern unsigned int iflushes;

/*
 * Pointers to code for particular CPU sub family.
 */
static void (*wb_inv_d_cache)(void);
static void (*inv_i_cache)(void);

#define CACHELINES	512	/* number of cachelines (kludgy)  */

extern inline void cache(unsigned int cacheop, unsigned long addr,
                          unsigned long offset, void *fault)
{
	__asm__ __volatile__ (
		"1:\tcache\t%0,%2+%1\n\t"
		".section\t__ex_table,\"a\"\n\t"
		STR(PTR)"\t1b,%3\n\t"
		".text"
		: /* no outputs */
		:"ri" (cacheop),
		 "o" (*(unsigned char *)addr),
		 "ri" (offset),
		 "ri" (fault));
}

/*
 * Code for R4000 style primary caches.
 *
 * R4000 style caches are direct-mapped, virtual indexed and physical tagged.
 * The size of cache line is either 16 or 32 bytes.
 * SC/MC versions of the CPUs add support for an second level cache with
 * upto 4mb configured as either joint or split I/D.  These level two
 * caches with direct support from CPU aren't yet supported.
 */

static void r4000_wb_inv_d_cache(void)
{
	unsigned long addr = KSEG0;
	int i;

	for (i=CACHELINES;i;i--) {
		cache(Index_Writeback_Inv_D, addr, 0, &&fault);
		addr += 32;
	}
	if (read_32bit_cp0_register(CP0_CONFIG) & CONFIG_DB)
		return;
	for (i=CACHELINES;i;i--) {
		cache(Index_Writeback_Inv_D, addr, 16, &&fault);
		addr += 32;
	}
fault:
}

static void r4000_inv_i_cache(void)
{
	unsigned long addr = KSEG0;
	int i;

	for (i=CACHELINES;i;i--) {
		cache(Index_Invalidate_I, addr, 0, &&fault);
		addr += 32;
	}
	if (read_32bit_cp0_register(CP0_CONFIG) & CONFIG_IB)
		return;
	for (i=CACHELINES;i;i--) {
		cache(Index_Invalidate_I, addr, 16, &&fault);
		addr += 32;
	}
fault:
}

/*
 * Code for R4600 style primary caches.
 *
 * R4600 has two way primary caches with 32 bytes line size.  The way to
 * flush is selected by bith 12 of the physical address given as argument
 * to an Index_* cache operation.  CPU supported second level caches are
 * not available.
 *
 * R4600 v1.0 bug:  Flush way 2, then way 1 of the instruction cache when
 * using Index_Invalidate_I.  IDT says this should work but is untested.
 * If this should not work, we have to disable interrupts for the broken
 * chips.  The CPU might otherwise execute code from the wrong cache way
 * during an interrupt.
 */
static void r4600_wb_inv_d_cache(void)
{
	unsigned long addr = KSEG0;
	int i;

	for (i=CACHELINES;i;i-=2) {
		cache(Index_Writeback_Inv_D, addr, 8192, &&fault);
		cache(Index_Writeback_Inv_D, addr, 0, &&fault);
		addr += 32;
	}
fault:
}

static void r4600_inv_i_cache(void)
{
	unsigned long addr = KSEG0;
	int i;

	for (i=CACHELINES;i;i-=2) {
		cache(Index_Invalidate_I, addr, 8192, &&fault);
		cache(Index_Invalidate_I, addr, 0, &&fault);
		addr += 32;
	}
fault:
}

/*
 * Flush the cache of R4x00.
 *
 * R4600 v2.0 bug: "The CACHE instructions Hit_Writeback_Invalidate_D,
 * Hit_Writeback_D, Hit_Invalidate_D and Create_Dirty_Exclusive_D will only
 * operate correctly if the internal data cache refill buffer is empty.  These
 * CACHE instructions should be separated from any potential data cache miss
 * by a load instruction to an uncached address to empty the response buffer."
 * (Revision 2.0 device errata from IDT available on http://www.idt.com/
 * in .pdf format.)
 *
 * To do: Use Hit_Invalidate where possible to be more economic.
 *        Handle SC & MC versions.
 *        The decission to nuke the entire cache might be based on a better
 *        decission algorithem based on the real costs.
 *        Handle different cache sizes.
 *        Combine the R4000 and R4600 cases.
 */
extern inline void
flush_d_cache(unsigned long addr, unsigned long size)
{
	unsigned long end;
	unsigned long a;

	dflushes++;
	if (1 || size >= dcache_size) {
		wb_inv_d_cache();
		return;
	}

	/*
	 * Workaround for R4600 bug.  Explanation see above.
	 */
	*(volatile unsigned long *)KSEG1;

	/*
	 * Ok, we only have to invalidate parts of the cache.
	 */
	a = addr & dcache_line_mask;
	end = (addr + size) & dcache_line_mask;
	while (1) {
		cache(Hit_Writeback_Inv_D, a, 0, &&fault);
		if (a == end) break;
		a += dcache_line_size;
	}
fault:
	return;
}

extern inline void
flush_i_cache(unsigned long addr, unsigned long size)
{
	unsigned long end;
	unsigned long a;

	iflushes++;
	if (1 || size >= icache_size) {
		inv_i_cache();
		return;
	}

	/*
	 * Ok, we only have to invalidate parts of the cache.
	 */
	a = addr & icache_line_mask;
	end = (addr + size) & dcache_line_mask;
	while (1) {
		cache(Hit_Invalidate_I, a, 0, &&fault);
		if (a == end) break;
		a += icache_line_size;
	}
fault:
	return;
}

asmlinkage void
mips3_cacheflush(unsigned long addr, unsigned long size, unsigned int flags)
{
	if (!(flags & CF_ALL))
		printk("mips3_cacheflush called without cachetype parameter\n");
	if (!(flags & CF_VIRTUAL))
		return; /* Nothing to do */
	if (flags & CF_DCACHE)
		flush_d_cache(addr, size);
	if (flags & CF_ICACHE)
		flush_i_cache(addr, size);
}

/* Going away. */
asmlinkage void fd_cacheflush(unsigned long addr, unsigned long size)
{
	cacheflush(addr, size, CF_DCACHE|CF_VIRTUAL);
}

void mips3_cache_init(void)
{
	extern asmlinkage void handle_vcei(void);
	extern asmlinkage void handle_vced(void);
	unsigned int c0_config = read_32bit_cp0_register(CP0_CONFIG);

	switch (mips_cputype) {
	case CPU_R4000MC: case CPU_R4400MC:
	case CPU_R4000SC: case CPU_R4400SC:
		/*
		 * Handlers not implemented yet.
		 */
		set_except_vector(14, handle_vcei);
		set_except_vector(31, handle_vced);
		break;
	default:
	}

	/*
	 * Which CPU are we running on?  There are different styles
	 * of primary caches in the MIPS R4xx0 CPUs.
	 */
	switch (mips_cputype) {
	case CPU_R4000MC: case CPU_R4400MC:
	case CPU_R4000SC: case CPU_R4400SC:
	case CPU_R4000PC: case CPU_R4400PC:
		inv_i_cache = r4000_inv_i_cache;
		wb_inv_d_cache = r4000_wb_inv_d_cache;
		break;
	case CPU_R4600: case CPU_R4700:
		inv_i_cache = r4600_inv_i_cache;
		wb_inv_d_cache = r4600_wb_inv_d_cache;
		break;
	default:
		panic("Don't know about cache type ...");
	}
	cacheflush = mips3_cacheflush;

	/*
	 * Find the size of primary instruction and data caches.
	 * For most CPUs these sizes are the same.
	 */
	dcache_size = 1 << (12 + ((c0_config >> 6) & 7));
	icache_size = 1 << (12 + ((c0_config >> 9) & 7));
	page_colour_mask = (dcache_size - 1) & ~(PAGE_SIZE - 1);

	/*
	 * Cache line sizes
	 */
	dcache_line_size = (c0_config & CONFIG_DB) ? 32 : 16;
	dcache_line_mask = ~(dcache_line_size - 1);
	icache_line_size = (c0_config & CONFIG_IB) ? 32 : 16;
	icache_line_mask = ~(icache_line_size - 1);

	printk("Primary D-cache size %dkb bytes, %d byte lines.\n",
	       dcache_size >> 10, dcache_line_size);
	printk("Primary I-cache size %dkb bytes, %d byte lines.\n",
	       icache_size >> 10, icache_line_size);

	/*
	 * Second level cache.
	 * FIXME ...
	 */
	if (!(c0_config & CONFIG_SC)) {
		printk("S-cache detected.  This type of of cache is not "
		       "supported yet.\n");
	}
}
