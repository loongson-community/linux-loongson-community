#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/cacheops.h>

/* These are the functions hooked by the memory management function pointers */
void sb1_clear_page(void *page)
{
	/* JDCXXX - This should be bottlenecked by the write buffer, but these
	   things tend to be mildly unpredictable...should check this on the
	   performance model */

	/* We prefetch 4 lines ahead.  We're also "cheating" slightly here...
	   since we know we're on an SB1, we force the assembler to take 
	   64-bit operands to speed things up */
	__asm__ __volatile__(
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     addiu     $1, %0, %2  \n"  /* Calculate the end of the page to clear */
		"     pref       5,  0(%0)  \n"  /* Prefetch the first 4 lines */
		"     pref       5, 32(%0)  \n"  
		"     pref       5, 64(%0)  \n"  
		"     pref       5, 96(%0)  \n"  
		"1:   sd        $0,  0(%0)  \n"  /* Throw out a cacheline of 0's */
		"     sd        $0,  8(%0)  \n"
		"     sd        $0, 16(%0)  \n"
		"     sd        $0, 24(%0)  \n"
		"     pref       5,128(%0)  \n"  /* Prefetch 4 lines ahead     */
		"     bne       $1, %0, 1b  \n"
		"     addiu     %0, %0, 32  \n"  /* Next cacheline (This instruction better be short piped!) */
		".set pop                   \n"
		: "=r" (page)
		: "0" (page), "I" (PAGE_SIZE-32)
		: "memory");

}

void sb1_copy_page(void *to, void *from)
{

	/* This should be optimized in assembly...can't use ld/sd, though,
	 * because the top 32 bits could be nuked if we took an interrupt
	 * during the routine.  And this is not a good place to be cli()'ing
	 */

	/* The pref's used here are using "streaming" hints, which cause the
	 * copied data to be kicked out of the cache sooner.  A page copy often
	 * ends up copying a lot more data than is commonly used, so this seems
	 * to make sense in terms of reducing cache pollution, but I've no real
	 * performance data to back this up
	 */ 

	__asm__ __volatile__(
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     addiu     $1, %0, %4  \n"  /* Calculate the end of the page to copy */
		"     pref       4,  0(%0)  \n"  /* Prefetch the first 3 lines to be read and copied */
		"     pref       5,  0(%1)  \n"  
		"     pref       4, 32(%0)  \n"  
		"     pref       5, 32(%1)  \n"  
		"     pref       4, 64(%0)  \n"  
		"     pref       5, 64(%1)  \n"  
		"1:   lw        $2,  0(%0)  \n"  /* Block copy a cacheline */
		"     lw        $3,  4(%0)  \n"
		"     lw        $4,  8(%0)  \n"
		"     lw        $5, 12(%0)  \n"
		"     lw        $6, 16(%0)  \n"
		"     lw        $7, 20(%0)  \n"
		"     lw        $8, 24(%0)  \n"
		"     lw        $9, 28(%0)  \n"
		"     pref       4, 96(%0)  \n"  /* Prefetch ahead         */
		"     pref       5, 96(%1)  \n"
		"     sw        $2,  0(%1)  \n"  
		"     sw        $3,  4(%1)  \n"
		"     sw        $4,  8(%1)  \n"
		"     sw        $5, 12(%1)  \n"
		"     sw        $6, 16(%1)  \n"
		"     sw        $7, 20(%1)  \n"
		"     sw        $8, 24(%1)  \n"
		"     sw        $9, 28(%1)  \n"		
		"     addiu     %1, %1, 32  \n"  /* Next cacheline */
		"     nop                   \n"  /* Force next add to short pipe */
		"     nop                   \n"  /* Force next add to short pipe */
		"     bne       $1, %0, 1b  \n"
		"     addiu     %0, %0, 32  \n"  /* Next cacheline */
		".set pop                   \n" 
		: "=r" (to), "=r" (from)
		: "0" (from), "1" (to), "I" (PAGE_SIZE-32)
		: "$2","$3","$4","$5","$6","$7","$8","$9","memory");
/*
	unsigned long *src = from;
	unsigned long *dest = to;
	unsigned long *target = (unsigned long *) (((unsigned long)src) + PAGE_SIZE);
	while (src != target) {
		*dest++ = *src++;
	}
*/
}

void pgd_init(unsigned long page)
{
	unsigned long *p = (unsigned long *) page;
	int i;

	for (i = 0; i < USER_PTRS_PER_PGD; i+=8) {
		p[i + 0] = (unsigned long) invalid_pte_table;
		p[i + 1] = (unsigned long) invalid_pte_table;
		p[i + 2] = (unsigned long) invalid_pte_table;
		p[i + 3] = (unsigned long) invalid_pte_table;
		p[i + 4] = (unsigned long) invalid_pte_table;
		p[i + 5] = (unsigned long) invalid_pte_table;
		p[i + 6] = (unsigned long) invalid_pte_table;
		p[i + 7] = (unsigned long) invalid_pte_table;
	}
}
