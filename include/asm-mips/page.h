#ifndef _ASM_MIPS_LINUX_PAGE_H
#define _ASM_MIPS_LINUX_PAGE_H

/*
 * For now...
 */
#define invalidate()

			/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT			12
#define PGDIR_SHIFT			22
#define PAGE_SIZE			(1UL << PAGE_SHIFT)
#define PGDIR_SIZE			(1UL << PGDIR_SHIFT)

#ifdef __KERNEL__

			/* number of bits that fit into a memory pointer */
#define BITS_PER_PTR			(8*sizeof(unsigned long))
			/* to mask away the intra-page address bits */
#define PAGE_MASK			(~(PAGE_SIZE-1))
			/* to mask away the intra-page address bits */
#define PGDIR_MASK			(~(PGDIR_SIZE-1))
			/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)		(((addr)+PAGE_SIZE-1)&PAGE_MASK)
			/* to align the pointer to a pointer address */
#define PTR_MASK			(~(sizeof(void*)-1))

					/* sizeof(void*)==1<<SIZEOF_PTR_LOG2 */
					/* 64-bit machines, beware!  SRB. */
#define SIZEOF_PTR_LOG2			2

			/* to find an entry in a page-table-directory */
#define PAGE_DIR_OFFSET(base,address)	((unsigned long*)((base)+\
  ((unsigned long)(address)>>(PAGE_SHIFT-SIZEOF_PTR_LOG2)*2&PTR_MASK&~PAGE_MASK)))
			/* to find an entry in a page-table */
#define PAGE_PTR(address)		\
  ((unsigned long)(address)>>(PAGE_SHIFT-SIZEOF_PTR_LOG2)&PTR_MASK&~PAGE_MASK)
			/* the no. of pointers that fit on a page */
#define PTRS_PER_PAGE			(PAGE_SIZE/sizeof(void*))

#define copy_page(from,to) \
	__copy_page((void *)(from),(void *)(to), PAGE_SIZE)

#if defined (__R4000__)
/*
 * Do it the 64bit way...
 */
extern __inline__ void __copy_page(void *from, void *to, int bytes)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n"
		"1:\tld\t$1,(%0)\n\t"
		"addiu\t%0,%0,8\n\t"
		"sd\t$1,(%1)\n\t"
		"subu\t%2,%2,8\n\t"
		"bne\t$0,%2,1b\n\t"
		"addiu\t%1,%1,8\n\t"
		".set\tat\n\t"
		".set\treorder\n\t"
		: "=r" (from), "=r" (to), "=r" (bytes)
		: "r"  (from), "r"  (to), "r"  (bytes)
		: "$1");
}
#else
/*
 * Use 32 bit Diesel fuel...
 */
extern __inline__ void __copy_page(void *from, void *to, int bytes)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n"
		"1:\tlw\t$1,(%0)\n\t"
		"addiu\t%0,%0,4\n\t"
		"sw\t$1,(%1)\n\t"
		"subu\t%2,%2,4\n\t"
		"bne\t$0,%2,1b\n\t"
		"addiu\t%1,%1,4\n\t"
		".set\tat\n\t"
		".set\treorder\n\t"
		: "=r" (from), "=r" (to), "=r" (bytes)
		: "r"  (from), "r"  (to), "r"  (bytes)
		: "$1");
}
#endif

#endif /* __KERNEL__ */

#endif /* _ASM_MIPS_LINUX_PAGE_H */
