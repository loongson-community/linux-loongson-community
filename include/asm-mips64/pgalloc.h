/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2001 by Ralf Baechle at alii
 * Copyright (C) 1999, 2000, 2001 Silicon Graphics, Inc.
 */
#ifndef _ASM_PGALLOC_H
#define _ASM_PGALLOC_H

#include <asm/pgtable.h>
#include <linux/config.h>

/* TLB flushing:
 *
 *  - flush_tlb_all() flushes all processes TLB entries
 *  - flush_tlb_mm(mm) flushes the specified mm context TLB entries
 *  - flush_tlb_page(vma, vmaddr) flushes a single page
 *  - flush_tlb_range(vma, start, end) flushes a range of pages
 *  - flush_tlb_pgtables(mm, start, end) flushes a range of page tables
 */
extern void local_flush_tlb_all(void);
extern void local_flush_tlb_mm(struct mm_struct *mm);
extern void local_flush_tlb_range(struct vm_area_struct *vma,
	unsigned long start, unsigned long end);
extern void local_flush_tlb_page(struct vm_area_struct *vma,
	unsigned long page);

#ifdef CONFIG_SMP

extern void flush_tlb_all(void);
extern void flush_tlb_mm(struct mm_struct *mm);
extern void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
	unsigned long end);
extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long);

#else /* CONFIG_SMP */

#define flush_tlb_all()			local_flush_tlb_all()
#define flush_tlb_mm(mm)		local_flush_tlb_mm(mm)
#define flush_tlb_range(vma,vmaddr,end)	local_flush_tlb_range(vma, vmaddr, end)
#define flush_tlb_page(vma,page)	local_flush_tlb_page(vma, page)

#endif /* CONFIG_SMP */

static inline void flush_tlb_pgtables(struct mm_struct *mm,
                                      unsigned long start, unsigned long end)
{
	/* Nothing to do on MIPS.  */
}

#define pmd_populate_kernel(mm, pmd, pte)	set_pmd(pmd, __pmd(pte))
#define pmd_populate(mm, pmd, pte)		set_pmd(pmd, __pmd(pte))
#define pgd_populate(mm, pgd, pmd)		set_pgd(pgd, __pgd(pmd))

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret, *init;

	ret = (pgd_t *) __get_free_pages(GFP_KERNEL, 1);
	if (ret) {
		init = pgd_offset(&init_mm, 0);
		pgd_init((unsigned long)ret);
		memcpy(ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
		       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
	}

	return ret;
}

static inline void pgd_free(pgd_t *pgd)
{
	free_pages((unsigned long)pgd, PGD_ORDER);
}

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
	unsigned long address)
{
	int count = 0;
	pte_t *pte;

	do {
		pte = (pte_t *) __get_free_pages(GFP_KERNEL, PTE_ORDER);
		if (pte)
			clear_page(pte);
		else {
			current->state = TASK_UNINTERRUPTIBLE;
			schedule_timeout(HZ);
			}
	} while (!pte && (count++ < 10));

	return pte;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm,
	unsigned long address)
{
	int count = 0;
	struct page *pte;

	do {
		pte = alloc_pages(GFP_KERNEL, PTE_ORDER);
		if (pte)
			clear_highpage(pte);
		else {
			current->state = TASK_UNINTERRUPTIBLE;
			schedule_timeout(HZ);
		}
	} while (!pte && (count++ < 10));

	return pte;
}

static inline void pte_free_kernel(pte_t *pte)
{
	free_pages((unsigned long)pte, PTE_ORDER);
}

static inline void pte_free(struct page *pte)
{
	__free_pages(pte, PTE_ORDER);
}

static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long address)
{
	pmd_t *pmd;

	pmd = (pmd_t *) __get_free_pages(GFP_KERNEL, PMD_ORDER);
	if (pmd)
		pmd_init((unsigned long)pmd, (unsigned long)invalid_pte_table);
	return pmd;
}

static inline void pmd_free(pmd_t *pmd)
{
	free_pages((unsigned long)pmd, PMD_ORDER);
}

extern pte_t kptbl[(PAGE_SIZE << PGD_ORDER)/sizeof(pte_t)];
extern pmd_t kpmdtbl[PTRS_PER_PMD];

#endif /* _ASM_PGALLOC_H */
