/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 Ralf Baechle
 *
 * Functions that manipulate entire pages.
 *
 * Not nice to have all these functions in two versions for cpus with
 * different cache line size but it seems to be by far the fastest thing
 * to schedule the cache instructions immediately before the store
 * instructions.  For example using the clear_page() version for 16 byte
 * lines on machine with 32 byte lines gives a measured penalty of
 * ~1280 cycles per page.
 */
#include <linux/mm.h>
#include <asm/cache.h>
#include <asm/mipsconfig.h>
#include <asm/page.h>
#include <asm/pgtable.h>

extern unsigned int dflushes;

/*
 * Initialize new page directory with pointers to invalid ptes.
 */
void mips3_pgd_init_32byte_lines(unsigned long page)
{
	unsigned long dummy1, dummy2;

	/*
	 * We generate dirty lines in the datacache, overwrite them
	 * then writeback the cache.
	 */
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"dsll32\t$1,%2,0\n\t"
		"dsrl32\t%2,$1,0\n\t"
		"or\t%2,$1\n"
		"1:\tcache\t%5,(%0)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%2,8(%0)\n\t"
		"sd\t%2,16(%0)\n\t"
		"sd\t%2,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%5,-32(%0)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%2,-24(%0)\n\t"
		"sd\t%2,-16(%0)\n\t"
		"bne\t%0,%1,1b\n\t"
		"sd\t%2,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=&r" (dummy1),
		 "=&r" (dummy2)
		:"r" (((unsigned long) invalid_pte_table - PAGE_OFFSET) |
		       _PAGE_TABLE),
		 "0" (page),
		 "1" (page + PAGE_SIZE - 64),
		 "i" (Create_Dirty_Excl_D)
		:"$1");
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"1:\tcache\t%3,(%0)\n\t"
		"bne\t%0,%1,1b\n\t"
		"daddiu\t%0,32\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1)
		:"0" (page),
		 "r" (page + PAGE_SIZE - 32),
		 "i" (Hit_Writeback_D));
	dflushes++;

#if 0
	cacheflush(page, PAGE_SIZE, CF_DCACHE|CF_VIRTUAL);
#endif
}

/*
 * Initialize new page directory with pointers to invalid ptes
 */
void mips3_pgd_init_16byte_lines(unsigned long page)
{
	unsigned long dummy1, dummy2;

	/*
	 * We generate dirty lines in the datacache, overwrite them
	 * then writeback the cache.
	 */
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"dsll32\t$1,%2,0\n\t"
		"dsrl32\t%2,$1,0\n\t"
		"or\t%2,$1\n"
		"1:\tcache\t%5,(%0)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%2,8(%0)\n\t"
		"cache\t%5,16(%0)\n\t"
		"sd\t%2,16(%0)\n\t"
		"sd\t%2,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%5,-32(%0)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%2,-24(%0)\n\t"
		"cache\t%5,-16(%0)\n\t"
		"sd\t%2,-16(%0)\n\t"
		"bne\t%0,%1,1b\n\t"
		"sd\t%2,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=&r" (dummy1),
		 "=&r" (dummy2)
		:"r" (((unsigned long) invalid_pte_table - PAGE_OFFSET) |
		       _PAGE_TABLE),
		 "0" (page),
		 "1" (page + PAGE_SIZE - 64),
		 "i" (Create_Dirty_Excl_D)
		:"$1");
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"1:\tcache\t%3,(%0)\n\t"
		"bne\t%0,%1,1b\n\t"
		"daddiu\t%0,16\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1)
		:"0" (page),
		 "r" (page + PAGE_SIZE - 16),
		 "i" (Hit_Writeback_D));
	dflushes++;

#if 0
	cacheflush(page, PAGE_SIZE, CF_DCACHE|CF_VIRTUAL);
#endif
}

/*
 * Zero an entire page.
 */

void (*clear_page)(unsigned long page);

void mips3_clear_page_32byte_lines(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}

void mips3_clear_page_16byte_lines(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"cache\t%3,16(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"cache\t%3,-16(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}

/*
 * This is still inefficient.  We only can do better if we know the
 * virtual address where the copy will be accessed.
 */
void (*copy_page)(unsigned long to, unsigned long from);

void mips3_copy_page_32byte_lines(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tcache\t%9,(%0)\n\t"
		"ld\t%2,(%1)\n\t"
		"ld\t%3,8(%1)\n\t"
		"ld\t%4,16(%1)\n\t"
		"ld\t%5,24(%1)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%3,8(%0)\n\t"
		"sd\t%4,16(%0)\n\t"
		"sd\t%5,24(%0)\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"ld\t%2,-32(%1)\n\t"
		"ld\t%3,-24(%1)\n\t"
		"ld\t%4,-16(%1)\n\t"
		"ld\t%5,-8(%1)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%3,-24(%0)\n\t"
		"sd\t%4,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t%5,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

void mips3_copy_page_16byte_lines(unsigned long to, unsigned long from)
{
	unsigned dummy1, dummy2;
	unsigned long reg1, reg2;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%6\n"
		"1:\tcache\t%7,(%0)\n\t"
		"ld\t%2,(%1)\n\t"
		"ld\t%3,8(%1)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%3,8(%0)\n\t"
		"cache\t%7,16(%0)\n\t"
		"ld\t%2,16(%1)\n\t"
		"ld\t%3,24(%1)\n\t"
		"sd\t%2,16(%0)\n\t"
		"sd\t%3,24(%0)\n\t"
		"cache\t%7,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"ld\t%2,-32(%1)\n\t"
		"ld\t%3,-24(%1)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%3,-24(%0)\n\t"
		"cache\t%7,-16(%0)\n\t"
		"ld\t%2,-16(%1)\n\t"
		"ld\t%3,-8(%1)\n\t"
		"sd\t%2,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t%3,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}
