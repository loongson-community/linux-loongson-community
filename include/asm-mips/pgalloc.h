/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2001 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#ifndef _ASM_PGALLOC_H
#define _ASM_PGALLOC_H

#include <linux/config.h>
#include <linux/highmem.h>
#include <asm/fixmap.h>

#define pmd_populate_kernel(mm, pmd, pte)	set_pmd(pmd, __pmd(pte))
#define pmd_populate(mm, pmd, pte)		set_pmd(pmd, __pmd(pte))

/*
 * Initialize new page directory with pointers to invalid ptes
 */
extern void pgd_init(unsigned long page);

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret = (pgd_t *)__get_free_pages(GFP_KERNEL, PGD_ORDER), *init;

	if (ret) {
		init = pgd_offset(&init_mm, 0);
		pgd_init((unsigned long)ret);
		memcpy (ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
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
		pte = (pte_t *) __get_free_page(GFP_KERNEL);
		if (pte)
			clear_page(pte);
		else {
			current->state = TASK_UNINTERRUPTIBLE;
			schedule_timeout(HZ);
			}
	} while (!pte && (count++ < 10));

	return pte;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm, unsigned long address)
{
	int count = 0;
	struct page *pte;

	do {
#if CONFIG_HIGHPTE
		pte = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, 0);
#else
		pte = alloc_pages(GFP_KERNEL, 0);
#endif
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
	free_page((unsigned long)pte);
}

static inline void pte_free(struct page *pte)
{
	__free_page(pte);
}

/*
 * allocating and freeing a pmd is trivial: the 1-entry pmd is
 * inside the pgd, so has no extra memory associated with it.
 */
#define pmd_alloc_one(mm, addr)		({ BUG(); ((pmd_t *)2); })
#define pmd_free(x)			do { } while (0)

#define check_pgt_cache()	do { } while (0)

#endif /* _ASM_PGALLOC_H */
