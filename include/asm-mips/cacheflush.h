/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000, 01, 02 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
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
 *  - flush_icache_range(start, end) flush a range of instructions
 *
 *  - flush_cache_sigtramp() flush signal trampoline
 *  - flush_icache_all() flush the entire instruction cache
 */
extern void (*_flush_cache_all)(void);
extern void (*___flush_cache_all)(void);
extern void (*_flush_cache_mm)(struct mm_struct *mm);
extern void (*_flush_cache_range)(struct vm_area_struct *vm,
	unsigned long start, unsigned long end);
extern void (*_flush_cache_page)(struct vm_area_struct *vma, unsigned long page);
extern void (*_flush_page_to_ram)(struct page * page);
extern void (*_flush_icache_range)(unsigned long start, unsigned long end);
extern void (*_flush_icache_page)(struct vm_area_struct *vma,
                                  struct page *page);
extern void (*_flush_cache_sigtramp)(unsigned long addr);
extern void (*_flush_icache_all)(void);

#define flush_dcache_page(page)			do { } while (0)

#define flush_cache_all()		_flush_cache_all()
#define __flush_cache_all()		___flush_cache_all()
#define flush_cache_mm(mm)		_flush_cache_mm(mm)
#define flush_cache_range(vma,start,end) _flush_cache_range(vma,start,end)
#define flush_cache_page(vma,page)	_flush_cache_page(vma, page)
#define flush_page_to_ram(page)		_flush_page_to_ram(page)

#define flush_icache_range(start, end)	_flush_icache_range(start,end)
#define flush_icache_page(vma, page) 	_flush_icache_page(vma, page)
#define flush_icache_user_range(vma, page, addr, len)	\
					_flush_icache_page((vma), (page))

#define flush_cache_sigtramp(addr)	_flush_cache_sigtramp(addr)
#ifdef CONFIG_VTAG_ICACHE
#define flush_icache_all()		_flush_icache_all()
#else
#define flush_icache_all()		do { } while(0)
#endif

#endif /* __ASM_CACHEFLUSH_H */
