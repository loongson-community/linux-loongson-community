/*
 * 64 bit MIPS specific page handling.
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
void mips4_pgd_init(unsigned long page)
{
	unsigned long dummy1, dummy2;

	/*
	 * We generate dirty lines in the datacache, overwrite these lines
	 * with zeros and then flush the cache.  Sounds horribly complicated
	 * but is just a trick to avoid unnecessary loads of from memory
	 * and uncached stores which are very expensive.
	 * FIXME:  This is the same like the R4000 version.  We could do some
	 * R10000 trickery using caching mode "uncached accelerated".
	 */
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"dsll32\t$1,%2,0\n\t"
		"dsrl32\t%2,$1,0\n\t"
		"or\t%2,$1\n"
		"1:\t"
		"cache\t%5,(%0)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%2,8(%0)\n\t"
		"cache\t%5,16(%0)\n\t"
		"sd\t%2,16(%0)\n\t"
		"sd\t%2,24(%0)\n\t"
		"cache\t%5,32(%0)\n\t"
		"sd\t%2,32(%0)\n\t"
		"sd\t%2,40(%0)\n\t"
		"cache\t%5,48(%0)\n\t"
		"sd\t%2,48(%0)\n\t"
		"sd\t%2,56(%0)\n\t"
		"subu\t%1,1\n\t"
		"bnez\t%1,1b\n\t"
		"addiu\t%0,64\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=&r" (dummy1),
		 "=&r" (dummy2)
		:"r" (((unsigned long) invalid_pte_table - PAGE_OFFSET) |
		       _PAGE_TABLE),
		 "0" (page),
		 "1" (PAGE_SIZE/(sizeof(pmd_t)*16)),
		 "i" (Create_Dirty_Excl_D)
		:"$1");
	/*
	 * Now force writeback to ashure values are in the RAM.
	 */
	cacheflush(page, PAGE_SIZE, CF_DCACHE|CF_VIRTUAL);
}

void (*clear_page)(unsigned long page);

/*
 * To do: cache magic
 */
void mips4_clear_page(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tsd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE)
		:"$1","memory");
}

void (*copy_page)(unsigned long to, unsigned long from);

/*
 * This is horribly inefficient ...
 */
void mips4_copy_page(unsigned long to, unsigned long from)
{
	/*
	 * Force writeback of old page to memory.  We don't know the
	 * virtual address, so we have to flush the entire cache ...
	 */
	cacheflush(0, ~0, CF_DCACHE|CF_VIRTUAL);
	sync_mem();
	memcpy((void *) to,
	       (void *) (from + (PT_OFFSET - PAGE_OFFSET)), PAGE_SIZE);
	/*
	 * Now writeback the page again if colour has changed.
	 */
	if (page_colour(from) != page_colour(to))
		cacheflush(0, ~0, CF_DCACHE|CF_VIRTUAL);
}
