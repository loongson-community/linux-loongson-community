/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 1999 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000 Kanoj Sarcar (kanoj@sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/r10kcache.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

extern void except_vec1_r10k(void);

#define NTLB_ENTRIES       64
#define NTLB_ENTRIES_HALF  32

void local_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	unsigned long entry;

#ifdef DEBUG_TLB
	printk("[tlball]");
#endif

	__save_and_cli(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = get_entryhi() & ASID_MASK;
	set_entryhi(CKSEG0);
	set_entrylo0(0);
	set_entrylo1(0);

	entry = get_wired();

	/* Blast 'em all away. */
	while (entry < NTLB_ENTRIES) {
		set_index(entry);
		tlb_write_indexed();
		entry++;
	}
	set_entryhi(old_ctx);
	__restore_flags(flags);
}

void local_flush_tlb_mm(struct mm_struct *mm)
{
	if (CPU_CONTEXT(smp_processor_id(), mm) != 0) {
		unsigned long flags;

#ifdef DEBUG_TLB
		printk("[tlbmm<%d>]", mm->context);
#endif
		__save_and_cli(flags);
		get_new_mmu_context(mm, smp_processor_id());
		if(mm == current->mm)
			set_entryhi(CPU_CONTEXT(smp_processor_id(), mm)
				    & ASID_MASK);
		__restore_flags(flags);
	}
}

void local_flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
                           unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (CPU_CONTEXT(smp_processor_id(), mm) != 0) {
		unsigned long flags;
		int size;

#ifdef DEBUG_TLB
		printk("[tlbrange<%02x,%08lx,%08lx>]",
		       (mm->context & ASID_MASK), start, end);
#endif
		__save_and_cli(flags);
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if (size <= NTLB_ENTRIES_HALF) {
			int oldpid = (get_entryhi() & ASID_MASK);
			int newpid = (CPU_CONTEXT(smp_processor_id(), mm)
				      & ASID_MASK);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while(start < end) {
				int idx;

				set_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				tlb_probe();
				idx = get_index();
				set_entrylo0(0);
				set_entrylo1(0);
				set_entryhi(KSEG0);
				if(idx < 0)
					continue;
				tlb_write_indexed();
			}
			set_entryhi(oldpid);
		} else {
			get_new_mmu_context(mm, smp_processor_id());
			if(mm == current->mm)
				set_entryhi(CPU_CONTEXT(smp_processor_id(), mm)
					    & ASID_MASK);
		}
		__restore_flags(flags);
	}
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	if (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) != 0) {
		unsigned long flags;
		int oldpid, newpid, idx;

#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", vma->vm_mm->context, page);
#endif
		newpid = (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) &
			  ASID_MASK);
		page &= (PAGE_MASK << 1);
		__save_and_cli(flags);
		oldpid = (get_entryhi() & ASID_MASK);
		set_entryhi(page | newpid);
		tlb_probe();
		idx = get_index();
		set_entrylo0(0);
		set_entrylo1(0);
		set_entryhi(KSEG0);
		if (idx < 0)
			goto finish;
		tlb_write_indexed();

	finish:
		set_entryhi(oldpid);
		__restore_flags(flags);
	}
}

/* XXX Simplify this.  On the R10000 writing a TLB entry for an virtual
   address that already exists will overwrite the old entry and not result
   in TLB malfunction or TLB shutdown.  */
static void andes_update_mmu_cache(struct vm_area_struct * vma,
                                   unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx, pid;

	/*
	 * Handle debugger faulting in for debugee.
	 */
	if (current->active_mm != vma->vm_mm)
		return;

	pid = get_entryhi() & ASID_MASK;

	if ((pid != (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) & ASID_MASK))
	    || (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) == 0)) {
		printk(KERN_WARNING
		       "%s: Wheee, bogus tlbpid mmpid=%d tlbpid=%d\n",
		       __FUNCTION__, (int) (CPU_CONTEXT(smp_processor_id(),
		       vma->vm_mm) & ASID_MASK), pid);
	}

	__save_and_cli(flags);
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (pid));
	pgdp = pgd_offset(vma->vm_mm, address);
	tlb_probe();
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	set_entryhi(address | (pid));
	if (idx < 0) {
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	set_entryhi(pid);
	__restore_flags(flags);
}

void __init andes_tlb_init(void)
{
	_update_mmu_cache = andes_update_mmu_cache;

	/*
	 * You should never change this register:
	 *   - On R4600 1.7 the tlbp never hits for pages smaller than
	 *     the value in the c0_pagemask register.
	 *   - The entire mm handling assumes the c0_pagemask register to
	 *     be set for 4kb pages.
	 */
	write_32bit_cp0_register(CP0_PAGEMASK, PM_4K);
	write_32bit_cp0_register(CP0_WIRED, 0);
	write_32bit_cp0_register(CP0_FRAMEMASK, 0);

        /* From this point on the ARC firmware is dead.  */
	local_flush_tlb_all();

	/* Did I tell you that ARC SUCKS?  */

	memcpy((void *)KSEG1 + 0x080, except_vec1_r10k, 0x80);
}
