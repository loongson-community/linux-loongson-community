/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2001, 02 by Ralf Baechle
 * Copyright (C) 1999, 2000, 2001 Silicon Graphics, Inc.
 */
#ifndef __ASM_CACHEFLUSH_H
#define __ASM_CACHEFLUSH_H

#include <linux/config.h>

/* Keep includes the same across arches.  */
#include <linux/mm.h>

/* Cache flushing:
 *
 *  - flush_cache_all() flushes entire cache
 *  - flush_cache_mm(mm) flushes the specified mm context's cache lines
 *  - flush_cache_page(mm, vmaddr) flushes a single page
 *  - flush_cache_range(vma, start, end) flushes a range of pages
 *  - flush_page_to_ram(page) write back kernel page to ram
 */
extern void (*_flush_cache_all)(void);
extern void (*___flush_cache_all)(void);
extern void (*_flush_cache_mm)(struct mm_struct *mm);
extern void (*_flush_cache_range)(struct vm_area_struct *vma,
	unsigned long start, unsigned long end);
extern void (*_flush_cache_page)(struct vm_area_struct *vma, unsigned long page);
extern void (*_flush_page_to_ram)(struct page * page);
extern void (*_flush_icache_all)(void);
extern void (*_flush_icache_range)(unsigned long start, unsigned long end);
extern void (*_flush_icache_page)(struct vm_area_struct *vma, struct page *page);

#define flush_cache_all()		_flush_cache_all()
#define __flush_cache_all()		___flush_cache_all()
#define flush_dcache_page(page)		do { } while (0)

#ifdef CONFIG_CPU_R10000
/*
 * Since the r10k handles VCEs in hardware, most of the flush cache
 * routines are not needed. Only the icache on a processor is not
 * coherent with the dcache of the _same_ processor, so we must flush
 * the icache so that it does not contain stale contents of physical
 * memory. No flushes are needed for dma coherency, since the o200s
 * are io coherent. The only place where we might be overoptimizing
 * out icache flushes are from mprotect (when PROT_EXEC is added).
 */
extern void andes_flush_icache_page(unsigned long);
#define flush_cache_mm(mm)			do { } while(0)
#define flush_cache_range(vma,start,end)	do { } while(0)
#define flush_cache_page(vma,page)		do { } while(0)
#define flush_page_to_ram(page)			do { } while(0)
#define flush_icache_range(start, end)		_flush_cache_l1()
#define flush_icache_user_range(vma, page, addr, len)	\
	flush_icache_page((vma), (page))
#define flush_icache_page(vma, page)					\
do {									\
	if ((vma)->vm_flags & VM_EXEC)					\
		andes_flush_icache_page(page_address(page));		\
} while (0)

#else

#define flush_cache_mm(mm)		_flush_cache_mm(mm)
#define flush_cache_range(vma,start,end) _flush_cache_range(vma,start,end)
#define flush_cache_page(vma,page)	_flush_cache_page(vma, page)
#define flush_page_to_ram(page)		_flush_page_to_ram(page)
#define flush_icache_range(start, end)	_flush_icache_range(start, end)
#define flush_icache_user_range(vma, page, addr, len)	\
	 flush_icache_page((vma), (page))
#define flush_icache_page(vma, page)	_flush_icache_page(vma, page)

#endif /* !CONFIG_CPU_R10000 */

#ifdef CONFIG_VTAG_ICACHE
#define flush_icache_all()		_flush_icache_all()
#else
#define flush_icache_all()		do { } while(0)
#endif

/*
 * The foll cache flushing routines are MIPS specific.
 * flush_cache_l2 is needed only during initialization.
 */
extern void (*_flush_cache_sigtramp)(unsigned long addr);
extern void (*_flush_cache_l2)(void);
extern void (*_flush_cache_l1)(void);

#define flush_cache_sigtramp(addr)	_flush_cache_sigtramp(addr)
#define flush_cache_l2()		_flush_cache_l2()
#define flush_cache_l1()		_flush_cache_l1()

#endif /* __ASM_CACHEFLUSH_H */
