/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * sb1250.c: MMU and cache operations for the SB1250
 */

#include <asm/mmu_context.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>

#include <asm/pgtable.h>

/*#define SB1_CACHE_CONSERVATIVE*/
/*#define SB1_TLB_CONSERVATIVE  */

/* These are probed at ld_mmu time */
static unsigned int icache_size;
static unsigned int dcache_size;

static unsigned int icache_line_size;
static unsigned int dcache_line_size;

static unsigned int icache_assoc;
static unsigned int dcache_assoc;

static unsigned int icache_sets;
static unsigned int dcache_sets;
static unsigned int tlb_entries;

#define read_mips32_cp0_config1()                               \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
	".set\tnoreorder\n\t"                                   \
	".set\tnoat\n\t"                                        \
     	".word\t0x40018001\n\t"                                 \
	"move\t%0,$1\n\t"                                       \
	".set\tat\n\t"                                          \
	".set\treorder"                                         \
	:"=r" (__res));                                         \
        __res;})

static void sb1_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

	__save_and_cli(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = (get_entryhi() & 0xff);
	set_entrylo0(0);
	set_entrylo1(0);
	for (entry = 0; entry < tlb_entries; entry++) {
		set_entryhi(KSEG0 + (PAGE_SIZE << 1) * entry);
		set_index(entry);
		tlb_write_indexed();
	}
	set_entryhi(old_ctx);
	__restore_flags(flags);	
}

/*
 *  Use a bogus region of memory (starting at 0) to sanitize the TLB's.  
 *  Use increments of the maximum page size (16MB), and check for duplicate entries
 *  before doing a given write.  Then, when we're safe from collisions with the
 *  firmware, go back and give all the entries invalid addresses with the normal
 *  flush routine.
 */
void sb1_sanitize_tlb(void)
{
	int entry;
	long addr = 0;
	long inc = 1<<24;  /* 16MB */
	/* Save old context and create impossible VPN2 value */
	set_entrylo0(0);
	set_entrylo1(0);
	for (entry = 0; entry < tlb_entries; entry++) {
		do {
			addr += inc;
			set_entryhi(addr);
			tlb_probe();
		} while ((int)(get_index()) >= 0);
		set_index(entry);
		tlb_write_indexed();
	}
	/* Now that we know we're safe from collisions, we can safely flush
	   the TLB with the "normal" routine. */
	sb1_flush_tlb_all();
	
}

static void sb1_flush_tlb_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
#ifdef SB1_TLB_CONSERVATIVE
	sb1_flush_tlb_all();
#else
	unsigned long flags;
	int cpu;
	__save_and_cli(flags);
	cpu = smp_processor_id();
	if(CPU_CONTEXT(cpu, mm) != 0) {
		int size;
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if(size <= 64) {
			int oldpid = (get_entryhi() & 0xff);
			int newpid = (CPU_CONTEXT(cpu, mm) & 0xff);

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
				set_entryhi(KSEG0 + (idx << (PAGE_SHIFT+1)));
				if(idx < 0)
					continue;
				tlb_write_indexed();
			}
			set_entryhi(oldpid);
		} else {
			get_new_cpu_mmu_context(mm, cpu);
			if (mm == current->active_mm)
				set_entryhi(CPU_CONTEXT(cpu, mm) & 0xff);
		}
	}
	__restore_flags(flags);
#endif
}

static void sb1_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
#ifdef SB1_TLB_CONSERVATIVE
	sb1_flush_tlb_all();
#else
	unsigned long flags;
	int cpu = smp_processor_id();
	__save_and_cli(flags);
	if (CPU_CONTEXT(cpu, vma->vm_mm) != 0) {
		int oldpid, newpid, idx;
#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", CPU_CONTEXT(cpu, vma->vm_mm), page);
#endif
		newpid = (CPU_CONTEXT(cpu, vma->vm_mm) & 0xff);
		page &= (PAGE_MASK << 1);
		oldpid = (get_entryhi() & 0xff);
		set_entryhi(page | newpid);
		tlb_probe();
		idx = get_index();
		set_entrylo0(0);
		set_entrylo1(0);
		if(idx < 0)
			goto finish;
		/* Make sure all entries differ. */  
		set_entryhi(KSEG0+(idx<<(PAGE_SHIFT + 1)));
		tlb_write_indexed();
	finish:
		set_entryhi(oldpid);
	}
	__restore_flags(flags);
#endif
}


/* All entries common to a mm share an asid.  To effectively flush
   these entries, we just bump the asid. */
static void sb1_flush_tlb_mm(struct mm_struct *mm)
{
#ifdef SB1_TLB_CONSERVATIVE
	sb1_flush_tlb_all();
#else
	unsigned long flags;
	int cpu;
	__save_and_cli(flags);
	cpu = smp_processor_id();
	if (CPU_CONTEXT(cpu, mm) != 0) {
		get_new_cpu_mmu_context(mm, cpu);
		if (mm == current->active_mm) {
			set_entryhi(CPU_CONTEXT(cpu, mm) & 0xff);
		}
	}
	__restore_flags(flags);
#endif
}

/* Stolen from mips32 routines */

void sb1_update_mmu_cache(struct vm_area_struct *vma, unsigned long address, pte_t pte)
{
#ifdef SB1_TLB_CONSERVATIVE
	sb1_flush_tlb_all();
#else
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

	__save_and_cli(flags);


	pid = get_entryhi() & 0xff;

#ifdef DEBUG_TLB
	if((pid != (CPU_CONTEXT(cpu, vma->vm_mm) & 0xff)) || (CPU_CONTEXT(cpu, vma->vm_mm) == 0)) {
		printk("update_mmu_cache: Wheee, bogus tlbpid mmpid=%d tlbpid=%d\n",
		       (int) (CPU_CONTEXT(cpu, vma->vm_mm) & 0xff), pid);
	}
#endif

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
	if(idx < 0) {
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	set_entryhi(pid);
	__restore_flags(flags);
#endif
}


#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
#define SB1_PREF_LOAD_STREAMED_HINT "0"
#define SB1_PREF_STORE_STREAMED_HINT "1"
#else
#define SB1_PREF_LOAD_STREAMED_HINT "4"
#define SB1_PREF_STORE_STREAMED_HINT "5"
#endif

/* These are the functions hooked by the memory management function pointers */
static void sb1_clear_page(void *page)
{
	/* JDCXXX - This should be bottlenecked by the write buffer, but these
	   things tend to be mildly unpredictable...should check this on the
	   performance model */

	/* No write-hint type op available.  We might be able to go a bit faster
	   by tweaking the caches and using uncached accellerated mode, but
	   doesn't seem worth the complexity.  We could also try doing this
	   with a reserved data mover on the sb1250...*/


	__asm__ __volatile__(
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     addiu     $1, %0, %2  \n"  /* Calculate the end of the page to clear */
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ",  0(%0)  \n"  /* Prefetch the first 4 lines */
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ", 32(%0)  \n"  
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ", 64(%0)  \n"  
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ", 96(%0)  \n"  
		"1:   sd        $0,  0(%0)  \n"  /* Throw out a cacheline of 0's */
		"     sd        $0,  8(%0)  \n"
		"     sd        $0, 16(%0)  \n"
		"     sd        $0, 24(%0)  \n"
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ",128(%0)  \n"  /* Prefetch 4 lines ahead     */
		"     bne       $1, %0, 1b  \n"
		"     addiu     %0, %0, 32  \n"  /* Next cacheline (This instruction better be short piped!) */
		".set pop                   \n"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE-32)
		:"$1","memory");

}

static void sb1_copy_page(void *to, void *from)
{

	/* The pref's used here are using "streaming" hints, which cause the copied data to be kicked
	   out of the cache sooner.  A page copy often ends up copying a lot more data than is commonly
	   used, so this seems to make sense in terms of reducing cache pollution, but I've no 
	   real performance data to back this up */

	__asm__ __volatile__(
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     daddiu    $1, %0, %4  \n"  /* Calculate the end of the page to copy */
		"     pref       " SB1_PREF_LOAD_STREAMED_HINT  ",  0(%0)  \n"  /* Prefetch the first 3 lines */
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ",  0(%1)  \n"  
		"     pref       " SB1_PREF_LOAD_STREAMED_HINT  ",  32(%0) \n"
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ",  32(%1) \n"  
		"     pref       " SB1_PREF_LOAD_STREAMED_HINT  ",  64(%0) \n"
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ",  64(%1) \n"  
		"1:   ld        $2,  0(%0)  \n"  /* Block copy a cacheline */
		"     ld        $3,  8(%0)  \n"
		"     ld        $4, 16(%0)  \n"
		"     ld        $5, 24(%0)  \n"
		"     pref       " SB1_PREF_LOAD_STREAMED_HINT  ", 96(%0)  \n"  /* Prefetch ahead         */
		"     pref       " SB1_PREF_STORE_STREAMED_HINT ", 96(%1)  \n"
		"     sd        $2,  0(%1)  \n"  
		"     sd        $3,  8(%1)  \n"
		"     sd        $4, 16(%1)  \n"
		"     sd        $5, 24(%1)  \n"
		"     daddiu    %1, %1, 32  \n"  /* Next cacheline */
		"     nop                   \n"  /* Force next add to short pipe */
		"     nop                   \n"  /* Force next add to short pipe */
		"     bne       $1, %0, 1b  \n"
		"     addiu     %0, %0, 32  \n"  /* Next cacheline */
		".set pop                   \n" 
		:"=r" (to),
		"=r" (from)
		:
		"0" (from),
		"1" (to),
		"I" (PAGE_SIZE-32)
		:"$1","$2","$3","$4","$5","memory");
/*
	unsigned long *src = from;
	unsigned long *dest = to;
	unsigned long *target = (unsigned long *) (((unsigned long)src) + PAGE_SIZE);
	while (src != target) {
		*dest++ = *src++;
	}
*/
}

/*
 * The dcache is fully coherent to the system, with one
 * big caveat:  the instruction stream.  In other words,
 * if we miss in the icache, and have dirty data in the
 * L1 dcache, then we'll go out to memory (or the L2) and
 * get the not-as-recent data.
 * 
 * So the only time we have to flush the dcache is when
 * we're flushing the icache.  Since the L2 is fully
 * coherent to everything, including I/O, we never have
 * to flush it
 */

static void sb1_flush_cache_all(void)
{
	/* Haven't worried too much about speed here; given that
	   we're flushing the icache, the time to invalidate is
	   dwarfed by the time it's going to take to refill it.
	   Register usage:
	   
	   $1 - moving cache index
	   $2 - set count
	*/

	if (icache_sets) {
		if (dcache_sets) {
			__asm__ __volatile__ (
				".set push                  \n"
				".set noreorder             \n"
				".set noat                  \n"
				".set mips4                 \n"
				"     move   $1, %2         \n" /* Start at index 0 */
				"1:   cache  0x1, 0($1)     \n" /* WB/Invalidate this index */
				"     addiu  %1, %1, -1     \n" /* Decrement loop count */
				"     bnez   %1, 1b         \n" /* loop test */
				"     daddu   $1, $1, %0    \n" /* Next address JDCXXX - Should be short piped */
				".set pop                   \n"
				::"r" (dcache_line_size),
				"r" (dcache_sets * dcache_assoc),
				"r" (KSEG0)
				:"$1");
		}
		__asm__ __volatile__ (
			".set push                  \n"
			".set noreorder             \n"
			".set noat                  \n"
			".set mips4                 \n"
			"     move   $1, %2         \n" /* Start at index 0 */
                        "1:   cache  0, 0($1)       \n" /* Invalidate this index */
			"     addiu  %1, %1, -1     \n" /* Decrement loop count */
			"     bnez   %1, 1b         \n" /* loop test */
			"     daddu   $1, $1, %0    \n" /* Next address JDCXXX - Should be short piped */
			".set pop                   \n"
			:
			:"r" (icache_line_size),
			"r" (icache_sets * icache_assoc),
			"r" (KSEG0)
			:"$1");
	}
}


/* When flushing a range in the icache, we have to first writeback
   the dcache for the same range, so new ifetches will see any
   data that was dirty in the dcache */

static void sb1_flush_icache_range(unsigned long start, unsigned long end)
{
#ifdef SB1_CACHE_CONSERVATIVE
	sb1_flush_cache_all();
#else
	if (start == end) {
		return;
	}

	start &= ~((long)(dcache_line_size - 1));
	end   = (end - 1) & ~((long)(dcache_line_size - 1));
	
	/* The 16M number here is pretty arbitrary */
	if ((end-start) >= (16*1024*1024)) {
		sb1_flush_cache_all();
	} else {
		if (icache_sets) {
			if (dcache_sets) {
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
				unsigned long flags;
				local_irq_save(flags);
#endif
					       
				__asm__ __volatile__ (
					".set push                  \n"
					".set noreorder             \n"
					".set noat                  \n"
					".set mips4                 \n"
					"     move   $1, %0         \n" 
					"1:                         \n"
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
					"     lw     $0,   0($1)    \n" /* Bug 1370, 1368            */
					"     cache  0x15, 0($1)    \n" /* Hit-WB-inval this address */
#else
					"     cache  0x19, 0($1)    \n" /* Hit-WB this address */
#endif
					"     bne    $1, %1, 1b     \n" /* loop test */
					"      daddu $1, $1, %2    \n" /* next line */
					".set pop                   \n"
					::"r" (start),
					"r" (end),
					"r" (dcache_line_size)
					:"$1");
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
				local_irq_restore(flags);
#endif
			}
			__asm__ __volatile__ (
				".set push                  \n"
				".set noreorder             \n"
				".set noat                  \n"
				".set mips4                 \n"
				"     move   $1, %0         \n" 
				"1:   cache  0x10, 0($1)    \n" /* Hit-inval this address */
				"     bne    $1, %1, 1b     \n" /* loop test */
				"      daddu  $1, $1, %2    \n" /* next line */
				".set pop                   \n"
				::"r" (start),
				"r" (end),
				"r" (icache_line_size)
				:"$1");
		}
	}
#endif
}

/*
 * Anything that just flushes dcache state can be
 * ignored, as we're always coherent in dcache space.
 * This is just a dummy function that all the
 * nop'ed routines point to
 */
static void sb1_nop(void)
{
}


static inline void protected_flush_icache_line(unsigned long addr)
{
	__asm__ __volatile__(
		"    .set push                \n"
		"    .set noreorder           \n"
		"    .set mips4               \n"
		"1:  cache 0x10, (%0)         \n"
		"2:  .set pop                 \n"
		"    .section __ex_table,\"a\"\n"
		"     .dword  1b, 2b          \n"
		"     .previous"
		:
		: "r" (addr));
}

static inline void protected_writeback_dcache_line(unsigned long addr)
{
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
	unsigned long flags;
	local_irq_save(flags);
#endif
		
	__asm__ __volatile__(
		"    .set push                \n"
		"    .set noreorder           \n"
		"    .set mips4               \n"
		"1:                           \n"
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
		"    lw    $0,   (%0)         \n"  
		"    cache 0x15, (%0)         \n"
#else
		"    cache 0x19, (%0)         \n"
#endif
		"2:  .set pop                 \n"
		"    .section __ex_table,\"a\"\n"
		"     .dword  1b, 2b          \n"
		"     .previous"
		:
		: "r" (addr));
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
	local_irq_restore(flags);
#endif
}

/*
 * XXX - Still need to really understand this.  This is mostly just
 * derived from the r10k and r4k implementations, and seems to work
 * but things that "seem to work" when I don't understand *why* they
 * "seem to work" disturb me greatly...JDC
 */
static void sb1_flush_cache_sigtramp(unsigned long addr)
{
#ifdef SB1_CACHE_CONSERVATIVE
	sb1_flush_cache_all();
#else
	unsigned long daddr, iaddr;
	
	daddr = addr & ~(dcache_line_size - 1);
	protected_writeback_dcache_line(daddr);
	protected_writeback_dcache_line(daddr + dcache_line_size);
	iaddr = addr & ~(icache_line_size - 1);
	protected_flush_icache_line(iaddr);
	protected_flush_icache_line(iaddr + icache_line_size);
#endif
}


/*
 * This only needs to make sure stores done up to this
 * point are visible to other agents outside the CPU.  Given 
 * the coherent nature of the ZBus, all that's required here is 
 * a sync to make sure the data gets out to the caches and is
 * visible to an arbitrary A Phase from an external agent 
 *   
 * Actually, I'm not even sure that's necessary; the semantics
 * of this function aren't clear.  If it's supposed to serve as
 * a memory barrier, this is needed.  If it's only meant to 
 * prevent data from being invisible to non-cpu memory accessors
 * for some indefinite period of time (e.g. in a non-coherent 
 * dcache) then this function would be a complete nop.
 */
static void sb1_flush_page_to_ram(struct page *page)
{
	__asm__ __volatile__(
		"     sync  \n"  /* Short pipe */
		:::"memory");	
}

static void sb1_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
#ifdef SB1_CACHE_CONSERVATIVE
	sb1_flush_cache_all();
#else
	unsigned long addr;

	if ((vma->vm_mm->context == 0) || !(vma->vm_flags & VM_EXEC)) {
		return;
	}

	addr = (unsigned long)page_address(page);
	sb1_flush_icache_range(addr, addr + PAGE_SIZE);
#endif
}






/*
 *  Cache set values (from the mips64 spec)
 * 0 - 64
 * 1 - 128
 * 2 - 256
 * 3 - 512
 * 4 - 1024
 * 5 - 2048
 * 6 - 4096
 * 7 - Reserved
 */
  
static unsigned int decode_cache_sets(unsigned int config_field)
{
	if (config_field == 7) {
		/* JDCXXX - Find a graceful way to abort. */
		return 0;
	} 
	return (1<<(config_field + 6));
}

/*
 *  Cache line size values (from the mips64 spec)
 * 0 - No cache present.
 * 1 - 4 bytes
 * 2 - 8 bytes
 * 3 - 16 bytes
 * 4 - 32 bytes
 * 5 - 64 bytes
 * 6 - 128 bytes
 * 7 - Reserved
 */

static unsigned int decode_cache_line_size(unsigned int config_field)
{
	if (config_field == 0) {
		return 0;
	} else if (config_field == 7) {
		/* JDCXXX - Find a graceful way to abort. */
		return 0;
	} 
	return (1<<(config_field + 1));
}

/*
 * Relevant bits of the config1 register format (from the MIPS32/MIPS64 specs)
 *
 * 24:22 Icache sets per way
 * 21:19 Icache line size
 * 18:16 Icache Associativity
 * 15:13 Dcache sets per way
 * 12:10 Dcache line size
 * 9:7   Dcache Associativity
 */


static void probe_cache_sizes(void)
{
	u32 config1 = read_mips32_cp0_config1();
	icache_line_size = decode_cache_line_size((config1 >> 19) & 0x7);
	dcache_line_size = decode_cache_line_size((config1 >> 10) & 0x7);
	icache_sets = decode_cache_sets((config1 >> 22) & 0x7);
	dcache_sets = decode_cache_sets((config1 >> 13) & 0x7);
        icache_assoc = ((config1 >> 16) & 0x7) + 1;
	dcache_assoc = ((config1 >> 7) & 0x7) + 1;
	icache_size = icache_line_size * icache_sets * icache_assoc;
	dcache_size = dcache_line_size * dcache_sets * dcache_assoc;
	tlb_entries = ((config1 >> 25) & 0x3f) + 1;
}

void sb1_show_regs(struct pt_regs *regs)
{
	/* Saved main processor registers. */
	printk("$0 : %016lx %016lx %016lx %016lx\n",
	       0UL, regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("$4 : %016lx %016lx %016lx %016lx\n",
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8 : %016lx %016lx %016lx %016lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11]);
	printk("$12: %016lx %016lx %016lx %016lx\n",
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16: %016lx %016lx %016lx %016lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19]);
	printk("$20: %016lx %016lx %016lx %016lx\n",
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24: %016lx %016lx\n",
	       regs->regs[24], regs->regs[25]);
	printk("$28: %016lx %016lx %016lx %016lx\n",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31]);

	/* Saved cp0 registers. */
	printk("epc   : %016lx\nStatus: %08lx\nCause : %08lx\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause);
	printk("errpc : %016lx\n", read_64bit_cp0_register(CP0_ERROREPC));
}



/* This is called from loadmmu.c.  We have to set up all the
   memory management function pointers, as well as initialize
   the caches and tlbs */
void ld_mmu_sb1(void)
{
	probe_cache_sizes();

	_clear_page = sb1_clear_page;
	_copy_page = sb1_copy_page;
	
	_flush_cache_all = sb1_flush_cache_all;
	_flush_cache_sigtramp = sb1_flush_cache_sigtramp;
	_flush_page_to_ram = sb1_flush_page_to_ram;
	_flush_icache_page = sb1_flush_icache_page;
	_flush_icache_range = sb1_flush_icache_range;

	_flush_cache_mm = (void (*)(struct mm_struct *))sb1_nop;
	_flush_cache_range = (void (*)(struct mm_struct *, unsigned long, unsigned long))sb1_nop;
	_flush_cache_page = (void (*)(struct vm_area_struct *, unsigned long))sb1_nop;

	_flush_tlb_all = sb1_flush_tlb_all;
	_flush_tlb_mm = sb1_flush_tlb_mm;
	_flush_tlb_range = sb1_flush_tlb_range;
	_flush_tlb_page = sb1_flush_tlb_page;

	_show_regs = sb1_show_regs;

	update_mmu_cache = sb1_update_mmu_cache;
	
	
	cpu_data[0].asid_cache = ASID_FIRST_VERSION;
	
	/* JDCXXX I'm not sure whether these are necessary: is this the right 
	   place to initialize the tlb?  If it is, why is it done 
	   at this level instead of as common code in loadmmu()?  */
	flush_cache_all();
	sb1_sanitize_tlb();
	
	/* Turn on caching in kseg0 */
	set_cp0_config(CONF_CM_CMASK, 0x5);
}


