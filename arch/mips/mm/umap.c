/*
 * arch/mips/mm/umap.c
 *
 * (C) Copyright 1994 Linus Torvalds
 *
 * Modified for removing active mappings from any task.  This is required
 * for implementing the virtual graphics interface for direct rendering
 * on the SGI - miguel.
 */
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/shm.h>
#include <linux/errno.h>
#include <linux/mman.h>
#include <linux/string.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/page.h>

static inline void
remove_mapping_pte_range (pmd_t *pmd, unsigned long address, unsigned long size)
{
	pte_t *pte;
	unsigned long end;

	if (pmd_none (*pmd))
		return;
	if (pmd_bad (*pmd)){
		printk ("remove_graphics_pte_range: bad pmd (%08lx)\n", pmd_val (*pmd));
		pmd_clear (pmd);
		return;
	}
	pte = pte_offset (pmd, address);
	address &= ~PMD_MASK;
	end = address + size;
	if (end > PMD_SIZE)
		end = PMD_SIZE;
	do {
		pte_t entry = *pte;
		if (pte_present (entry))
			set_pte (pte, pte_modify (entry, PAGE_NONE));
		address += PAGE_SIZE;
		pte++;
	} while (address < end);
						  
}

static inline void
remove_mapping_pmd_range (pgd_t *pgd, unsigned long address, unsigned long size)
{
	pmd_t *pmd;
	unsigned long end;

	if (pgd_none (*pgd))
		return;

	if (pgd_bad (*pgd)){
		printk ("remove_graphics_pmd_range: bad pgd (%08lx)\n", pgd_val (*pgd));
		pgd_clear (pgd);
		return;
	}
	pmd = pmd_offset (pgd, address);
	address &= ~PGDIR_MASK;
	end = address + size;
	if (end > PGDIR_SIZE)
		end = PGDIR_SIZE;
	do {
		remove_mapping_pte_range (pmd, address, end - address);
		address = (address + PMD_SIZE) & PMD_MASK;
		pmd++;
	} while (address < end);
		
}

/*
 * This routine is called from the page fault handler to remove a
 * range of active mappings at this point
 */

void
remove_mapping (struct task_struct *task, unsigned long start, unsigned long end)
{
	unsigned long beg = start;
	pgd_t *dir;

	down (&task->mm->mmap_sem);
	dir = pgd_offset (task->mm, start);
	flush_cache_range (task->mm, beg, end);
	while (start < end){
		remove_mapping_pmd_range (dir, start, end - start);
		start = (start + PGDIR_SIZE) & PGDIR_MASK;
		dir++;
	}
	flush_tlb_range (task->mm, beg, end);
	up (&task->mm->mmap_sem);
}

