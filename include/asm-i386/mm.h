#ifndef _ASM_I386_MM_H
#define _ASM_I386_MM_H

#if defined (__KERNEL__)

#define PAGE_PRESENT    0x001
#define PAGE_RW         0x002
#define PAGE_USER       0x004
#define PAGE_PWT        0x008   /* 486 only - not used currently */
#define PAGE_PCD        0x010   /* 486 only - not used currently */
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_COW        0x200   /* implemented in software (one of the AVL bits) */

#define PAGE_PRIVATE    (PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_ACCESSED | PAGE_COW)
#define PAGE_SHARED     (PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_ACCESSED)
#define PAGE_COPY       (PAGE_PRESENT | PAGE_USER | PAGE_ACCESSED | PAGE_COW)
#define PAGE_READONLY   (PAGE_PRESENT | PAGE_USER | PAGE_ACCESSED)
#define PAGE_TABLE      (PAGE_PRESENT | PAGE_RW | PAGE_USER | PAGE_ACCESSED)

#define invalidate() \
__asm__ __volatile__("movl %%cr3,%%eax\n\tmovl %%eax,%%cr3": : :"ax")

extern inline long find_in_swap_cache (unsigned long addr)
{
	unsigned long entry;

#ifdef SWAP_CACHE_INFO
	swap_cache_find_total++;
#endif
	__asm__ __volatile__("xchgl %0,%1"
		:"=m" (swap_cache[addr >> PAGE_SHIFT]),
		 "=r" (entry)
		:"0" (swap_cache[addr >> PAGE_SHIFT]),
		 "1" (0));
#ifdef SWAP_CACHE_INFO
	if (entry)
		swap_cache_find_success++;
#endif	
	return entry;
}

extern inline int delete_from_swap_cache(unsigned long addr)
{
	unsigned long entry;
	
#ifdef SWAP_CACHE_INFO
	swap_cache_del_total++;
#endif	
	__asm__ __volatile__("xchgl %0,%1"
		:"=m" (swap_cache[addr >> PAGE_SHIFT]),
		 "=r" (entry)
		:"0" (swap_cache[addr >> PAGE_SHIFT]),
		 "1" (0));
	if (entry)  {
#ifdef SWAP_CACHE_INFO
		swap_cache_del_success++;
#endif
		swap_free(entry);
		return 1;
	}
	return 0;
}

/*
 * memory.c & swap.c
 */
extern void mem_init(unsigned long low_start_mem,
		     unsigned long start_mem, unsigned long end_mem);

#endif /* __KERNEL__ */

#endif /* _ASM_I386_MM_H */
