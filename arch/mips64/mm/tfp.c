/* $Id: tfp.c,v 1.5 2000/01/17 23:32:46 ralf Exp $
 *
 * tfp.c: MMU and cache routines specific to the r8000 (TFP).
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

static void tfp_clear_page(void * page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tsd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page), "I" (PAGE_SIZE)
		:"$1", "memory");
}

static void tfp_copy_page(void * to, void * from)
{
	unsigned long dummy1, dummy2, reg1, reg2;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%6\n"
		"1:\tld\t%2,(%1)\n\t"
		"ld\t%3,8(%1)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%3,8(%0)\n\t"
		"ld\t%2,16(%1)\n\t"
		"ld\t%3,24(%1)\n\t"
		"sd\t%2,16(%0)\n\t"
		"sd\t%3,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"ld\t%2,-32(%1)\n\t"
		"ld\t%3,-24(%1)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%3,-24(%0)\n\t"
		"ld\t%2,-16(%1)\n\t"
		"ld\t%3,-8(%1)\n\t"
		"sd\t%2,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		" sd\t%3,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2), "=&r" (reg1), "=&r" (reg2)
		:"0" (to), "1" (from), "I" (PAGE_SIZE));
}

/* Cache operations. XXX Write these dave... */
static inline void tfp_flush_cache_all(void)
{
	/* XXX */
}

static void tfp_flush_cache_mm(struct mm_struct *mm)
{
	/* XXX */
}

static void tfp_flush_cache_range(struct mm_struct *mm,
				  unsigned long start,
				  unsigned long end)
{
	/* XXX */
}

static void tfp_flush_cache_page(struct vm_area_struct *vma,
				 unsigned long page)
{
	/* XXX */
}

static void tfp_flush_page_to_ram(struct page * page)
{
	/* XXX */
}

static void tfp_flush_cache_sigtramp(unsigned long page)
{
	/* XXX */
}

/* TLB operations. XXX Write these dave... */
static inline void tfp_flush_tlb_all(void)
{
	/* XXX */
}

static void tfp_flush_tlb_mm(struct mm_struct *mm)
{
	/* XXX */
}

static void tfp_flush_tlb_range(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
	/* XXX */
}

static void tfp_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	/* XXX */
}

static int tfp_user_mode(struct pt_regs *regs)
{
	return (regs->cp0_status & ST0_KSU) == KSU_USER;
}

void __init ld_mmu_tfp(void)
{
	clear_page = tfp_clear_page;
	copy_page = tfp_copy_page;

	flush_cache_all = tfp_flush_cache_all;
	flush_cache_mm = tfp_flush_cache_mm;
	flush_cache_range = tfp_flush_cache_range;
	flush_cache_page = tfp_flush_cache_page;
	flush_cache_sigtramp = tfp_flush_cache_sigtramp;
	flush_page_to_ram = tfp_flush_page_to_ram;

	flush_tlb_all = tfp_flush_tlb_all;
	flush_tlb_mm = tfp_flush_tlb_mm;
	flush_tlb_range = tfp_flush_tlb_range;
	flush_tlb_page = tfp_flush_tlb_page;

	user_mode = tfp_user_mode;

	flush_cache_all();
	flush_tlb_all();
}
