/* $Id: mmu_context.h,v 1.3 1999/01/04 16:09:23 ralf Exp $
 *
 * Switch a MMU context.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998 by Ralf Baechle
 */
#ifndef __ASM_MIPS_MMU_CONTEXT_H
#define __ASM_MIPS_MMU_CONTEXT_H

#include <linux/config.h>

/* Fuck.  The f-word is here so you can grep for it :-)  */
extern unsigned long asid_cache;

#if defined(CONFIG_CPU_R3000)

#define ASID_INC	0x40
#define ASID_MASK	0xfc0

#else /* FIXME: not correct for R6000, R8000 */

#define ASID_INC	0x1
#define ASID_MASK	0xff

#endif

/* 
 *  All unused by hardware upper bits will be considered 
 *  as a software asid extension.
 */
#define ASID_VERSION_MASK  ((unsigned long)~(ASID_MASK|(ASID_MASK-1))) 
#define ASID_FIRST_VERSION ((unsigned long)(~ASID_VERSION_MASK) + 1)

extern inline void get_new_mmu_context(struct mm_struct *mm, unsigned
long asid)
{
        if (! ((asid += ASID_INC) & ASID_MASK) ) {
		flush_tlb_all(); /* start new asid cycle */
		if (!asid)      /* fix version if needed */
			asid = ASID_FIRST_VERSION;
	}
	mm->context = asid_cache = asid;
}

extern inline void
get_mmu_context(struct task_struct *p)
{
	struct mm_struct *mm = p->mm;

	if (mm) {
		unsigned long asid = asid_cache;
		/* Check if our ASID is of an older version and thus invalid */
		if ((mm->context ^ asid) & ASID_VERSION_MASK)
			get_new_mmu_context(mm, asid);
	}
}

/*
 * Initialize the context related info for a new mm_struct
 * instance.
 */
extern inline void init_new_context(struct mm_struct *mm)
{
	mm->context = 0;
}

/*
 * Destroy context related info for an mm_struct that is about
 * to be put to rest.
 */
extern inline void destroy_context(struct mm_struct *mm)
{
	mm->context = 0;
}

/*
 * After we have set current->mm to a new value, this activates
 * the context for the new mm so we see the new mappings.
 */
extern inline void activate_context(struct task_struct *tsk)
{
	get_mmu_context(tsk);
	set_entryhi(tsk->mm->context);
}

#endif /* __ASM_MIPS_MMU_CONTEXT_H */
