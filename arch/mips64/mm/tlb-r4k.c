/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2002 MIPS Technologies, Inc.  All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * MIPS64 CPU variant specific MMU routines.
 * These routine are not optimized in any way, they are done in a generic way
 * so they can be used on all MIPS64 compliant CPUs, and also done in an
 * attempt not to break anything for the R4xx0 style CPUs.
 */
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/system.h>

#undef DEBUG_TLB
#undef DEBUG_TLBUPDATE

extern char except_vec1_r4k;

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

void local_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

#ifdef DEBUG_TLB
	printk("[tlball]");
#endif

	__save_and_cli(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = (get_entryhi() & 0xff);
	set_entryhi(KSEG0);
	set_entrylo0(0);
	set_entrylo1(0);
	BARRIER;

	entry = get_wired();

	/* Blast 'em all away. */
	while(entry < mips_cpu.tlbsize) {
	        /* Make sure all entries differ. */
	        set_entryhi(KSEG0+entry*0x2000);
		set_index(entry);
		BARRIER;
		tlb_write_indexed();
		BARRIER;
		entry++;
	}
	BARRIER;
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
		if (mm == current->active_mm)
			set_entryhi(CPU_CONTEXT(smp_processor_id(), mm) &
				    0xff);
		__restore_flags(flags);
	}
}

void local_flush_tlb_range(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
	if (CPU_CONTEXT(smp_processor_id(), mm) != 0) {
		unsigned long flags;
		int size;

#ifdef DEBUG_TLB
		printk("[tlbrange<%02x,%08lx,%08lx>]", (mm->context & 0xff),
		       start, end);
#endif
		__save_and_cli(flags);
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if(size <= mips_cpu.tlbsize/2) {
			int oldpid = (get_entryhi() & 0xff);
			int newpid = (CPU_CONTEXT(smp_processor_id(), mm) &
				      0xff);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while(start < end) {
				int idx;

				set_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				BARRIER;
				tlb_probe();
				BARRIER;
				idx = get_index();
				set_entrylo0(0);
				set_entrylo1(0);
				if(idx < 0)
					continue;
				/* Make sure all entries differ. */
				set_entryhi(KSEG0+idx*0x2000);
				BARRIER;
				tlb_write_indexed();
				BARRIER;
			}
			set_entryhi(oldpid);
		} else {
			get_new_mmu_context(mm, smp_processor_id());
			if (mm == current->active_mm)
				set_entryhi(CPU_CONTEXT(smp_processor_id(),
							mm) & 0xff);
		}
		__restore_flags(flags);
	}
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	if (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) != 0) {
		unsigned long flags;
		unsigned long oldpid, newpid, idx;

#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", vma->vm_mm->context, page);
#endif
		newpid = (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) & 0xff);
		page &= (PAGE_MASK << 1);
		__save_and_cli(flags);
		oldpid = (get_entryhi() & 0xff);
		set_entryhi(page | newpid);
		BARRIER;
		tlb_probe();
		BARRIER;
		idx = get_index();
		set_entrylo0(0);
		set_entrylo1(0);
		if(idx < 0)
			goto finish;
		/* Make sure all entries differ. */
		set_entryhi(KSEG0+idx*0x2000);
		BARRIER;
		tlb_write_indexed();
	finish:
		BARRIER;
		set_entryhi(oldpid);
		__restore_flags(flags);
	}
}

/*
 * Updates the TLB with the new pte(s).
 */
void mips64_update_mmu_cache(struct vm_area_struct * vma,
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

	pid = get_entryhi() & 0xff;

#ifdef DEBUG_TLB
	if((pid != (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) & 0xff)) ||
	   (CPU_CONTEXT(smp_processor_id(), vma->vm_mm) == 0)) {
		printk("update_mmu_cache: Wheee, bogus tlbpid mmpid=%d
			tlbpid=%d\n", (int) (CPU_CONTEXT(smp_processor_id(),
			vma->vm_mm) & 0xff), pid);
	}
#endif

	__save_and_cli(flags);
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (pid));
	pgdp = pgd_offset(vma->vm_mm, address);
	BARRIER;
	tlb_probe();
	BARRIER;
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	BARRIER;
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	set_entryhi(address | (pid));
	BARRIER;
	if(idx < 0) {
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	BARRIER;
	set_entryhi(pid);
	BARRIER;
	__restore_flags(flags);
}

void dump_mm_page(unsigned long addr)
{
	pgd_t	*page_dir, *pgd;
	pmd_t	*pmd;
	pte_t	*pte, page;
	unsigned long val;

	page_dir = pgd_offset(current->mm, 0);
	pgd = pgd_offset(current->mm, addr);
	pmd = pmd_offset(pgd, addr);
	pte = pte_offset(pmd, addr);
	page = *pte;
	printk("Memory Mapping: VA = %08x, PA = %08x ", addr, (unsigned int) pte_val(page));
	val = pte_val(page);
	if (val & _PAGE_PRESENT) printk("present ");
	if (val & _PAGE_READ) printk("read ");
	if (val & _PAGE_WRITE) printk("write ");
	if (val & _PAGE_ACCESSED) printk("accessed ");
	if (val & _PAGE_MODIFIED) printk("modified ");
	if (val & _PAGE_R4KBUG) printk("r4kbug ");
	if (val & _PAGE_GLOBAL) printk("global ");
	if (val & _PAGE_VALID) printk("valid ");
	printk("\n");
}

void show_tlb(void)
{
        unsigned int flags;
        unsigned int old_ctx;
	unsigned int entry;
	unsigned int entrylo0, entrylo1, entryhi;

	__save_and_cli(flags);

	/* Save old context */
	old_ctx = (get_entryhi() & 0xff);

	printk("TLB content:\n");
	entry = 0;
	while(entry < mips_cpu.tlbsize) {
		set_index(entry);
		BARRIER;
		tlb_read();
		BARRIER;
		entryhi = get_entryhi();
		entrylo0 = get_entrylo0();
		entrylo1 = get_entrylo1();
		printk("%02d: ASID=%02d%s VA=0x%08x ", entry, entryhi & ASID_MASK, (entrylo0 & entrylo1 & 1) ? "(G)" : "   ", entryhi & ~ASID_MASK);
		printk("PA0=0x%08x C0=%x %s%s%s\n", (entrylo0>>6)<<12, (entrylo0>>3) & 7, (entrylo0 & 4) ? "Dirty " : "", (entrylo0 & 2) ? "Valid " : "Invalid ", (entrylo0 & 1) ? "Global" : "");
		printk("\t\t\t     PA1=0x%08x C1=%x %s%s%s\n", (entrylo1>>6)<<12, (entrylo1>>3) & 7, (entrylo1 & 4) ? "Dirty " : "", (entrylo1 & 2) ? "Valid " : "Invalid ", (entrylo1 & 1) ? "Global" : "");

		dump_mm_page(entryhi & ~0xff);
		dump_mm_page((entryhi & ~0xff) | 0x1000);
		entry++;
	}
	BARRIER;
	set_entryhi(old_ctx);

	__restore_flags(flags);
}

void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
				      unsigned long entryhi, unsigned long pagemask)
{
        unsigned long flags;
        unsigned long wired;
        unsigned long old_pagemask;
        unsigned long old_ctx;

        __save_and_cli(flags);
        /* Save old context and create impossible VPN2 value */
        old_ctx = (get_entryhi() & 0xff);
        old_pagemask = get_pagemask();
        wired = get_wired();
        set_wired (wired + 1);
        set_index (wired);
        BARRIER;
        set_pagemask (pagemask);
        set_entryhi(entryhi);
        set_entrylo0(entrylo0);
        set_entrylo1(entrylo1);
        BARRIER;
        tlb_write_indexed();
        BARRIER;

        set_entryhi(old_ctx);
        BARRIER;
        set_pagemask (old_pagemask);
        local_flush_tlb_all();
        __restore_flags(flags);
}

/*
 * Used for loading TLB entries before trap_init() has started, when we
 * don't actually want to add a wired entry which remains throughout the
 * lifetime of the system
 */

static int temp_tlb_entry __initdata;

__init int add_temporary_entry(unsigned long entrylo0, unsigned long entrylo1,
			       unsigned long entryhi, unsigned long pagemask)
{
	int ret = 0;
	unsigned long flags;
	unsigned long wired;
	unsigned long old_pagemask;
	unsigned long old_ctx;

	__save_and_cli(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = get_entryhi() & 0xff;
	old_pagemask = get_pagemask();
	wired = get_wired();
	if (--temp_tlb_entry < wired) {
		printk(KERN_WARNING "No TLB space left for add_temporary_entry\n");
		ret = -ENOSPC;
		goto out;
	}

	set_index(temp_tlb_entry);
	BARRIER;
	set_pagemask(pagemask);
	set_entryhi(entryhi);
	set_entrylo0(entrylo0);
	set_entrylo1(entrylo1);
	BARRIER;
	tlb_write_indexed();
	BARRIER;

	set_entryhi(old_ctx);
	BARRIER;
	set_pagemask(old_pagemask);
out:
	__restore_flags(flags);
	return ret;
}

static void __init probe_tlb(unsigned long config)
{
        unsigned long config1;

        if (!(config & (1 << 31))) {
	        /*
		 * Not a MIPS64 complainant CPU.
		 * Config 1 register not supported, we assume R4k style.
		 */
	        mips_cpu.tlbsize = 48;
	} else {
	        config1 = read_mips32_cp0_config1();
		if (!((config >> 7) & 3))
		        panic("No MMU present");
		else
		        mips_cpu.tlbsize = ((config1 >> 25) & 0x3f) + 1;
	}

	printk("Number of TLB entries %d.\n", mips_cpu.tlbsize);
}

void __init r4k_tlb_init(void)
{
	unsigned long config = read_32bit_cp0_register(CP0_CONFIG);

	probe_tlb(config);
	_update_mmu_cache = mips64_update_mmu_cache;
	set_pagemask(PM_4K);
	write_32bit_cp0_register(CP0_WIRED, 0);
	temp_tlb_entry = mips_cpu.tlbsize - 1;
	local_flush_tlb_all();

	memcpy((void *)(KSEG0 + 0x80), &except_vec1_r4k, 0x80);
	flush_icache_range(KSEG0, KSEG0 + 0x80);
}
