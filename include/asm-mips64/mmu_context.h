/*
 * Switch a MMU context.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_MMU_CONTEXT_H
#define _ASM_MMU_CONTEXT_H

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

/*
 * For the fast tlb miss handlers, we currently keep a per cpu array
 * of pointers to the current pgd for each processor. Also, the proc.
 * id is stuffed into the context register. This should be changed to
 * use the processor id via current->processor, where current is stored
 * in watchhi/lo. The context register should be used to contiguously
 * map the page tables.
 */
#define TLBMISS_HANDLER_SETUP_PGD(pgd) \
	pgd_current[smp_processor_id()] = (unsigned long)(pgd)
#define TLBMISS_HANDLER_SETUP() \
	set_context(((long)(&pgd_current[smp_processor_id()])) << 23); \
	TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir)
extern unsigned long pgd_current[];

#define ASID_INC	0x1
#define ASID_MASK	0xff

#ifdef CONFIG_SMP
#define cpu_context(cpu, mm)	((mm)->context[cpu])
#else
#define cpu_context(cpu, mm)	((mm)->context)
#endif
#define cpu_asid(cpu, mm)	(cpu_context((cpu), (mm)) & ASID_MASK)
#define asid_cache(cpu)		(cpu_data[cpu].asid_cache)

static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk, unsigned cpu)
{
}

/*
 *  All unused by hardware upper bits will be considered
 *  as a software asid extension.
 */
#define ASID_VERSION_MASK  ((unsigned long)~(ASID_MASK|(ASID_MASK-1)))
#define ASID_FIRST_VERSION ((unsigned long)(~ASID_VERSION_MASK) + 1)

static inline void
get_new_mmu_context(struct mm_struct *mm, unsigned long cpu)
{
	unsigned long asid = asid_cache(cpu);

	if (! ((asid += ASID_INC) & ASID_MASK) ) {
		flush_icache_all();
		local_flush_tlb_all();	/* start new asid cycle */
		if (!asid)		/* fix version if needed */
			asid = ASID_FIRST_VERSION;
	}
	cpu_context(cpu, mm) = asid_cache(cpu) = asid;
}

/*
 * Initialize the context related info for a new mm_struct
 * instance.
 */
static inline int
init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
#ifdef CONFIG_SMP
	mm->context = kmalloc(smp_num_cpus * sizeof(unsigned long), GFP_KERNEL);
	/*
 	 * Init the "context" values so that a tlbpid allocation
	 * happens on the first switch.
 	 */
	if (mm->context == 0)
		return -ENOMEM;
	memset(mm->context, 0, smp_num_cpus * sizeof(unsigned long));
#else
	mm->context = 0;
#endif
	return 0;
}

static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
                             struct task_struct *tsk, unsigned cpu)
{
	/* Check if our ASID is of an older version and thus invalid */
	if ((cpu_context(cpu, next) ^ asid_cache(cpu)) & ASID_VERSION_MASK)
		get_new_mmu_context(next, cpu);

	set_entryhi(cpu_context(cpu, next));
	TLBMISS_HANDLER_SETUP_PGD(next->pgd);
}

/*
 * Destroy context related info for an mm_struct that is about
 * to be put to rest.
 */
static inline void destroy_context(struct mm_struct *mm)
{
#ifdef CONFIG_SMP
	if (mm->context)
		kfree(mm->context);
#endif
}

/*
 * After we have set current->mm to a new value, this activates
 * the context for the new mm so we see the new mappings.
 */
static inline void
activate_mm(struct mm_struct *prev, struct mm_struct *next)
{
	/* Unconditionally get a new ASID.  */
	get_new_mmu_context(next, smp_processor_id());

	set_entryhi(cpu_context(smp_processor_id(), next));
	TLBMISS_HANDLER_SETUP_PGD(next->pgd);
}

#endif /* _ASM_MMU_CONTEXT_H */
