#ifndef _ASM_MIPS_MM_H_
#define _ASM_MIPS_MM_H_

#if defined (__KERNEL__)

/*
 * Note that we shift the lower 32bits of each EntryLo[01] entry
 * 6 bit to the left. That way we can convert the PFN into the
 * physical address by a single and operation and gain 6 aditional
 * bits for storing information which isn't present in a normal
 * MIPS page table.
 * I've also changed the naming of some bits so that they conform
 * the i386 naming as much as possible.
 */
#define PAGE_COW                   (1<<0)   /* implemented in software */
#define PAGE_ACCESSED              (1<<1)   /* implemented in software */
#define PAGE_DIRTY                 (1<<2)   /* implemented in software */
#define PAGE_USER                  (1<<3)   /* implemented in software */
#define PAGE_UNUSED2               (1<<4)   /* for use by software     */
#define PAGE_UNUSED3               (1<<5)   /* for use by software     */
#define PAGE_GLOBAL                (1<<6)
#define PAGE_VALID                 (1<<7)
/*
 * In the hardware the PAGE_WP bit is represented by the dirty bit
 */
#define PAGE_RW                    (1<<8)
#define CACHE_CACHABLE_NO_WA       (0<<9)
#define CACHE_CACHABLE_WA          (1<<9)
#define CACHE_UNCACHED             (2<<9)
#define CACHE_CACHABLE_NONCOHERENT (3<<9)
#define CACHE_CACHABLE_CE          (4<<9)
#define CACHE_CACHABLE_COW         (5<<9)
#define CACHE_CACHABLE_CUW         (6<<9)
#define CACHE_MASK                 (7<<9)

#define PAGE_PRIVATE    (PAGE_VALID | PAGE_ACCESSED | PAGE_DIRTY | \
                         PAGE_RW | PAGE_COW)
#define PAGE_SHARED     (PAGE_VALID | PAGE_ACCESSED | PAGE_DIRTY | PAGE_RW)
#define PAGE_COPY       (PAGE_VALID | PAGE_ACCESSED | PAGE_COW)
#define PAGE_READONLY   (PAGE_VALID | PAGE_ACCESSED)
#define PAGE_TABLE      (PAGE_VALID | PAGE_ACCESSED | PAGE_DIRTY | PAGE_RW)

/*
 * Predicate for testing
 */
#define IS_PAGE_USER(p) (((unsigned long)(p)) & PAGE_USER)

extern inline long find_in_swap_cache (unsigned long addr)
{
	unsigned long entry;

#ifdef SWAP_CACHE_INFO
	swap_cache_find_total++;
#endif
	cli();
	entry = swap_cache[addr >> PAGE_SHIFT];
	swap_cache[addr >> PAGE_SHIFT] = 0;
	sti();
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
	cli();
	entry = swap_cache[addr >> PAGE_SHIFT];
	swap_cache[addr >> PAGE_SHIFT] = 0;
	sti();
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
extern void mem_init(unsigned long start_mem, unsigned long end_mem);

#endif /* defined (__KERNEL__) */

#endif /* _ASM_MIPS_MM_H_ */
