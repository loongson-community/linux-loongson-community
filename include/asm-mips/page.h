#ifndef __ASM_MIPS_PAGE_H
#define __ASM_MIPS_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__

#define CONFIG_STRICT_MM_TYPECHECKS

#ifndef __LANGUAGE_ASSEMBLY__

#include <asm/cachectl.h>

#ifdef CONFIG_STRICT_MM_TYPECHECKS
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

#else /* !defined (CONFIG_STRICT_MM_TYPECHECKS) */
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

#endif /* !defined (CONFIG_STRICT_MM_TYPECHECKS) */

#include <linux/linkage.h>

extern asmlinkage void tlbflush(void);
#define invalidate()	({sys_cacheflush(0, ~0, BCACHE);tlbflush();})

#if __mips == 3
typedef unsigned int mem_map_t;
#else
typedef unsigned short mem_map_t;
#endif

#endif /* __LANGUAGE_ASSEMBLY__ */

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/* This handles the memory map */
#if __mips == 3
/*
 * We handle pages at XKPHYS + 0x1800000000000000 (cachable, noncoherent)
 * Pagetables are at  XKPHYS + 0x1000000000000000 (uncached)
 */
#define PAGE_OFFSET	0x9800000000000000UL
#define PT_OFFSET	0x9000000000000000UL
#define MAP_MASK        0x07ffffffffffffffUL
#define MAP_PAGE_RESERVED (1<<31)
#else
/*
 * We handle pages at KSEG0 (cachable, noncoherent)
 * Pagetables are at  KSEG1 (uncached)
 */
#define PAGE_OFFSET	0x80000000
#define PT_OFFSET	0xa0000000
#define MAP_MASK        0x1fffffff
#define MAP_PAGE_RESERVED (1<<15)
#endif

#define MAP_NR(addr)	((((unsigned long)(addr)) & MAP_MASK) >> PAGE_SHIFT)

#ifndef __LANGUAGE_ASSEMBLY__

#define copy_page(from,to) __copy_page((unsigned long)from, (unsigned long)to)

extern unsigned long page_colour_mask;

extern inline unsigned long
page_colour(unsigned long page)
{
	return page & page_colour_mask;
}

#if 0
extern inline void __copy_page(unsigned long from, unsigned long to)
{
printk("__copy_page(%08lx, %08lx)\n", from, to);
	sys_cacheflush(0, ~0, DCACHE);
	sync_mem();
	from += (PT_OFFSET - PAGE_OFFSET);
	to += (PT_OFFSET - PAGE_OFFSET);
	memcpy((void *) to, (void *) from, PAGE_SIZE);
	sys_cacheflush(0, ~0, ICACHE);
}
#else
extern void __copy_page(unsigned long from, unsigned long to);
#endif

#endif /* defined (__LANGUAGE_ASSEMBLY__) */
#endif /* defined (__KERNEL__) */

#endif /* __ASM_MIPS_PAGE_H */
