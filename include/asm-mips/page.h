/* $Id: page.h,v 1.4 1998/08/25 09:21:59 ralf Exp $
 *
 * Definitions for page handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1998 by Ralf Baechle
 */
#ifndef __ASM_MIPS_PAGE_H
#define __ASM_MIPS_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__

#define STRICT_MM_TYPECHECKS

#ifndef _LANGUAGE_ASSEMBLY

#ifdef __SMP__
#define ULOCK_DECLARE extern spinlock_t user_page_lock;
#else
#define ULOCK_DECLARE
#endif
struct upcache {
	struct page *list;
	unsigned long count;
};
extern struct upcache user_page_cache[8];
#define USER_PAGE_WATER		16

extern unsigned long get_user_page_slow(int which);
extern unsigned long user_page_colours, user_page_order;

#define get_user_page(__vaddr) \
({ \
	ULOCK_DECLARE \
	int which = ((__vaddr) >> PAGE_SHIFT) & user_page_colours; \
	struct upcache *up = &user_page_cache[which]; \
	struct page *p; \
	unsigned long ret; \
	spin_lock(&user_page_lock); \
	if((p = up->list) != NULL) { \
		up->list = p->next; \
		up->count--; \
	} \
	spin_unlock(&user_page_lock); \
	if(p != NULL) \
		ret = PAGE_OFFSET+PAGE_SIZE*p->map_nr; \
	else \
		ret = get_user_page_slow(which); \
	ret; \
})

#define free_user_page(__page, __addr) \
do { \
	ULOCK_DECLARE \
	int which = ((__addr) >> PAGE_SHIFT) & user_page_colours; \
	struct upcache *up = &user_page_cache[which]; \
	if(atomic_read(&(__page)->count) == 1 && \
           up->count < USER_PAGE_WATER) { \
		spin_lock(&user_page_lock); \
		(__page)->age = PAGE_INITIAL_AGE; \
		(__page)->next = up->list; \
		up->list = (__page); \
		up->count++; \
		spin_unlock(&user_page_lock); \
	} else \
		free_page(__addr); \
} while(0)

extern void (*clear_page)(unsigned long page);
extern void (*copy_page)(unsigned long to, unsigned long from);

#ifdef STRICT_MM_TYPECHECKS
/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) } )
#define __pme(x)	((pme_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

#else /* !defined (STRICT_MM_TYPECHECKS) */
/*
 * .. while these make it easier on the compiler
 */
typedef unsigned long pte_t;
typedef unsigned long pmd_t;
typedef unsigned long pgd_t;
typedef unsigned long pgprot_t;

#define pte_val(x)	(x)
#define pmd_val(x)	(x)
#define pgd_val(x)	(x)
#define pgprot_val(x)	(x)

#define __pte(x)	(x)
#define __pmd(x)	(x)
#define __pgd(x)	(x)
#define __pgprot(x)	(x)

#endif /* !defined (STRICT_MM_TYPECHECKS) */

#endif /* _LANGUAGE_ASSEMBLY */

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/*
 * This handles the memory map.
 * We handle pages at KSEG0 for kernels with 32 bit address space.
 */
#define PAGE_OFFSET	0x80000000UL
#define __pa(x)		((unsigned long) (x) - PAGE_OFFSET)
#define __va(x)		((void *)((unsigned long) (x) + PAGE_OFFSET))
#define MAP_NR(addr)	(__pa(addr) >> PAGE_SHIFT)

#endif /* defined (__KERNEL__) */

#endif /* __ASM_MIPS_PAGE_H */
