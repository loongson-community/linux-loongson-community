/*
 * 32 bit MIPS specific page handling.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 Ralf Baechle
 */
#include <linux/mm.h>
#include <asm/cache.h>
#include <asm/mipsconfig.h>
#include <asm/page.h>
#include <asm/pgtable.h>

void (*pgd_init)(unsigned long page);

/*
 * Initialize new page directory with pointers to invalid ptes
 */
void mips2_pgd_init(unsigned long page)
{
	unsigned long dummy1, dummy2;

	/*
	 * This version is optimized for the R6000.  We generate dirty lines
	 * in the datacache, overwrite these lines with zeros and then flush
	 * the cache.  Sounds horribly complicated but is just a trick to
	 * avoid unnecessary loads of from memory and uncached stores which
	 * are very expensive.  Not tested yet as the R6000 is a rare CPU only
	 * available in SGI machines and I don't have one.
	 */
	__asm__ __volatile__(
		".set\tnoreorder\n"
		"1:\t"
		"cache\t%5,(%0)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%2,4(%0)\n\t"
		"sw\t%2,8(%0)\n\t"
		"sw\t%2,12(%0)\n\t"
		"cache\t%5,16(%0)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%2,20(%0)\n\t"
		"sw\t%2,24(%0)\n\t"
		"sw\t%2,28(%0)\n\t"
		"subu\t%1,1\n\t"
		"bnez\t%1,1b\n\t"
		"addiu\t%0,32\n\t"
		".set\treorder"
		:"=r" (dummy1),
		 "=r" (dummy2)
		:"r" (((unsigned long) invalid_pte_table - PAGE_OFFSET) |
		       _PAGE_TABLE),
		 "0" (page),
		 "1" (PAGE_SIZE/(sizeof(pmd_t)*8)),
		 "i" (Create_Dirty_Excl_D));
#endif
	/*
	 * Now force writeback to ashure values are in the RAM.
	 */
	cacheflush(page, PAGE_SIZE, CF_DCACHE|CF_PHYSICAL);
}

void (*clear_page)(unsigned long page);

/*
 * To do: cache magic, maybe FPU for 64 accesses when clearing cache pages.
 */
void mips2_clear_page(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"addiu\t$1,%0,%2\n"
		"1:\tsw\t$0,(%0)\n\t"
		"sw\t$0,4(%0)\n\t"
		"sw\t$0,8(%0)\n\t"
		"sw\t$0,12(%0)\n\t"
		"addiu\t%0,32\n\t"
		"sw\t$0,-16(%0)\n\t"
		"sw\t$0,-12(%0)\n\t"
		"sw\t$0,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t$0,-4(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE)
		:"$1","memory");
}

void (*copy_page)(unsigned long to, unsigned long from);

void mips2_copy_page(unsigned long to, unsigned long from)
{
	memcpy((void *) to,
	       (void *) (from + (PT_OFFSET - PAGE_OFFSET)), PAGE_SIZE);
}
