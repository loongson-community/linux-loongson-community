/*
 * Definitions for page handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle
 */
#ifndef __ASM_MIPS_PAGE_H
#define __ASM_MIPS_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__

#define STRICT_MM_TYPECHECKS

extern void (*clear_page)(unsigned long page);
extern void (*copy_page)(unsigned long to, unsigned long from);

#ifndef __LANGUAGE_ASSEMBLY__

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

#endif /* __LANGUAGE_ASSEMBLY__ */

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/* This handles the memory map */
#if 0
/*
 * Kernel with 64 bit address space.
 * We handle pages at XKPHYS + 0x1800000000000000 (cachable, noncoherent)
 * Pagetables are at  XKPHYS + 0x1000000000000000 (uncached)
 */
#define PAGE_OFFSET	0x9800000000000000UL
#define PT_OFFSET	0x9000000000000000UL
#define MAP_MASK        0x07ffffffffffffffUL
#else /* !defined (__mips64) */
/*
 * Kernel with 32 bit address space.
 * We handle pages at KSEG0 (cachable, noncoherent)
 * Pagetables are at  KSEG1 (uncached)
 */
#define PAGE_OFFSET	0x80000000UL
#define PT_OFFSET	0xa0000000UL
#define MAP_MASK        0x1fffffffUL
#endif /* !defined (__mips64) */

/*
 * __pa breaks when applied to a pagetable pointer on >= R4000.
 */
#define __pa(x)			((unsigned long) (x) - PAGE_OFFSET)
#define __va(x)			((void *)((unsigned long) (x) + PAGE_OFFSET))

#define MAP_NR(addr)	((((unsigned long)(addr)) & MAP_MASK) >> PAGE_SHIFT)

#ifndef __LANGUAGE_ASSEMBLY__

extern unsigned long page_colour_mask;

extern inline unsigned long
page_colour(unsigned long page)
{
	return page & page_colour_mask;
}

#endif /* defined (__LANGUAGE_ASSEMBLY__) */
#endif /* defined (__KERNEL__) */

#endif /* __ASM_MIPS_PAGE_H */
