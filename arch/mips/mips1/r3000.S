/*
 * arch/mips/kernel/r3000.S
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics, 1996 Paul M. Antoine
 * Written by Ralf Baechle and Andreas Busse
 * Modified for R3000 by Paul M. Antoine
 *
 *  Additional R3000 support by Didier Frick <dfrick@dial.eunet.ch>
 *  for ACN S.A, Copyright (C) 1996 by ACN S.A
 *
 * This file contains most of the R3000/R3000A specific routines, which would
 * probably work on the R2000 (if anyone's interested!).
 *
 * This code is evil magic. Read appendix f (coprocessor 0 hazards) of
 * all R3000/MIPS manuals and think about that MIPS means "Microprocessor without
 * Interlocked Pipeline Stages" before you even think about changing this code!
 *
 * Then remember that some bugs here are due to my not having completely
 * converted the R4xx0 code to R3000 and that the R4xx0 CPU's are more
 * forgiving than the R3000/A!!  All that, and the fact that I'm not up to
 * 'guru' level on R3000 - PMA.
 * (Paul, I replaced all occurances of TLBMAPHI with %HI(TLBMAP) -- Ralf)
 */
#include <linux/config.h>

#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/fpregdef.h>
#include <asm/mipsconfig.h>
#include <asm/mipsregs.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

#ifdef __SMP__
#error "Fix this for SMP!"
#else
#define current current_set
#endif

/*

FIXME:
	- First of all, this really screams for a light version of SAVE_ALL
	  and RESTORE_ALL, saving and restoring only the context actually 
	  needed in this case. I'm afraid it's necessary to save some context
          on the stack because on the R3000 tlb exceptions can nest in some
           cases where they wouldn't on the R4000.

	- The TLB handling code should be completely rewritten for the R3000
	   because too many things are different from the R4000.
   	   For instance, the CP0_CONTEXT register has a different format
	   and cannot be reused with the current setup.
           I really had to do a fast hack to get it to work, but no time to do
	   it cleanly for now, sorry.
	   We also introduced a tlb_softindex variable to point to the next
	   TLB entry to write. This variable is incremented everytime we add a
           new entry to the TLB. We did this because we felt that using the 
           CP0_RANDOM register could be unsafe in some cases (like trashing
           the TLB entry for the handler's return address in user space).
           It's very possible that we are wrong on this one, but we had so 
	   much trouble with this TLB thing that we chose the safe side.
*/

#define CONF_DEBUG_TLB
#undef CONFIG_TLB_SHUTDOWN
#undef TLB_LOG

MODE_ALIAS	=	0x00e0				# cachable

		.text
		.set	mips1
		.set	noreorder

		.align	5
		NESTED(handle_tlbl, FR_SIZE, sp)
		.set	noat
		/*
		 * Check whether this is a refill or an invalid exception
		 */
		mfc0	k0,CP0_BADVADDR
		nop
		mfc0	k1,CP0_ENTRYHI
		ori	k0,0xfff			# clear ASID...
		xori	k0,0xfff			# in BadVAddr
		andi	k1,0xfc0			# get current ASID
		or	k0,k1				# make new entryhi
		mfc0	k1,CP0_ENTRYHI
		nop
		mtc0	k0,CP0_ENTRYHI
		nop					# for pipeline
		tlbp
		nop					# for pipeline
		mfc0	k0,CP0_INDEX
		nop
		mtc0	k1,CP0_ENTRYHI			# delay slot
		bgez	k0,invalid_tlbl			# bad addr in c0_badvaddr
		nop


		mfc0	k0,CP0_BADVADDR
		lui	k1,0xe000
		subu	k0,k0,k1
		bgez	k0,1f
		nop
		j	real_utlb
		nop	
		
1:


#ifdef CONF_DEBUG_TLB
		/*
		 * OK, this is a double fault. Let's see whether this is
		 * due to an invalid entry in the page_table.
		 */

		lw	k0, tlbl_lock
		nop
		bnez	k0,1f
		li	k1,1
		la	k0, tlbl_lock
		sw	k1,(k0)


		mfc0	k0,CP0_BADVADDR
		lui	k1,58368
		srl	k0,12				# get PFN?
		sll	k0,2
		addu	k0,k1
		lw	k1,(k0)
		nop
		andi	k1,(_PAGE_PRESENT|_PAGE_ACCESSED)
		bnez	k1,reload_pgd_entries
		nop					# delay slot

1:
		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp)

		PRINT("Double fault caused by invalid entries in pgd:\n")
		mfc0	a1,CP0_BADVADDR
		nop
		PRINT("Double fault address     : %08lx\n")
		mfc0	a1,CP0_EPC
		nop
		PRINT("c0_epc                   : %08lx\n")
		jal	show_regs
		move	a0,sp
		jal	dump_tlb_nonwired
		nop
		mfc0	a0,CP0_BADVADDR
		jal	dump_list_current
		nop
		.set	noat
		STI
		.set	at
		PANIC("Corrupted pagedir")
		.set	noat

reload_pgd_entries:
#endif /* CONF_DEBUG_TLB */

		/*
		 * Load missing pair of entries from the pgd and return.
		 */

		mfc0	k0,CP0_BADVADDR			
		nop
		lui	k1,58368

		srl	k0,12				
		sll	k0,2
		addu	k0,k1
		lw	k0,(k0)
		nop
		mtc0	k0,CP0_ENTRYLO0

		la	k0, tlb_softIndex
		lw	k1,(k0)
		nop
		mtc0	k1,CP0_INDEX
		nop
		addu	k1,(1<<8)
		andi	k0,k1,(63<<8)
		bnez	k0, 1f
		nop
		li	k1,(8<<8)
1:		
		la	k0, tlb_softIndex
		sw	k1,(k0)


		nop
		nop
		nop				# for pipeline
		tlbwi
		nop				# for pipeline
		nop
		nop
		

#ifdef CONF_DEBUG_TLB
		la	k0, tlbl_lock
		sw	zero,(k0)
#endif
		mfc0	k0,CP0_EPC
		nop
		jr	k0
		rfe
		nop
		

		/*
		 * Handle invalid exception
		 *
		 * There are two possible causes for an invalid (tlbl)
		 * exception:
		 * 1) pages with present bit set but the valid bit clear
		 * 2) nonexistant pages
		 * Case one needs fast handling, therefore don't save
		 * registers yet.
		 *
		 * k0 contains c0_index.
		 */
invalid_tlbl:	
		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp)

#ifdef TLB_LOG
		PRINT	("tlbl: invalid\n");
		nop	
#endif
		/*
		 * Test present bit in entry
		 */
		lw	s0,FR_BADVADDR(sp)
		nop
		srl	s0,12
		sll	s0,2
		lui	k1,58368
		addu	s0,k1
		
		lw	k1,(s0)
		nop
		andi	k1,(_PAGE_PRESENT|_PAGE_READ)
		xori	k1,(_PAGE_PRESENT|_PAGE_READ)
		bnez	k1,nopage_tlbl
		nop
		/*
		 * Present and read bits are set -> set valid and accessed bits
		 */
		lw	k1,(s0)		# delay slot
		nop	
		ori	k1,(_PAGE_VALID|_PAGE_ACCESSED)
		sw	k1,(s0)

		mtc0	k1,CP0_ENTRYLO0
		nop
		tlbwi
		nop
		nop

		j	return
		nop

		/*
		 * Page doesn't exist. Lots of work which is less important
		 * for speed needs to be done, so hand it all over to the
		 * kernel memory management routines.
		 */
nopage_tlbl:	
/*		SAVE_ALL	*/
		REG_S	sp,FR_ORIG_REG2(sp) */
#ifdef TLB_LOG
		PRINT	("nopage_tlbl\n");
		nop	
#endif
#ifdef CONFIG_TLB_SHUTDOWN
		mfc0	t0,CP0_INDEX
		sll	t0,4
		la	t1,KSEG1
		or	t0,t1
		mtc0	t0,CP0_ENTRYHI
		mtc0	zero,CP0_ENTRYLO0
		nop
		nop
		tlbwi
		nop
		nop
#endif

		lw	a2,FR_BADVADDR(sp)
		li	t1,-1			# not a sys call
		sw	t1,FR_ORIG_REG2(sp)
		nop
		STI
		.set	at
		/*
		 * a0 (struct pt_regs *) regs
		 * a1 (unsigned long)    0 for read access
		 * a2 (unsigned long)    faulting virtual address
		 */
		move	a0,sp
		jal	do_page_fault
		li	a1,0				# delay slot
		j	ret_from_sys_call
		nop					# delay slot
		END(handle_tlbl)

		.text
		.align	5
		NESTED(handle_tlbs, FR_SIZE, sp)
		.set	noat
		/*
		 * It is impossible that is a nested reload exception.
		 * Therefore this must be a invalid exception.
		 * Two possible cases:
		 * 1) Page exists but not dirty.
		 * 2) Page doesn't exist yet. Hand over to the kernel.
		 *
		 * Test whether present bit in entry is set
		 */
		/* used to be dmfc0 */

#ifdef CONF_DEBUG_TLB

		la	k0,tlbs_lock
		lw	k1,(k0)
		nop
		beqz	k1,3f
		nop
		.set	noat
		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp)
		.set	at
		PRINT("Nested tlbs exception:\n")
		mfc0	a1,CP0_BADVADDR
		nop
		PRINT("Virtual address     : %08lx\n")
		mfc0	a1,CP0_EPC
		nop
		PRINT("c0_epc              : %08lx\n")
		jal	show_regs
		move	a0,sp
		jal	dump_tlb_nonwired
		nop
		mfc0	a0,CP0_BADVADDR
		jal	dump_list_current
		nop
		.set	noat
		STI
		.set	at
		PANIC("Nested tlbs exception")

3:
		li	k1,1
		sw	k1,(k0)

#endif
		.set	noat
		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp)
		.set	at

		mfc0	s0,CP0_BADVADDR

		lui	k1,58368
		srl	s0,12
		sll	s0,2
		addu	s0,k1
		nop
		lw	k1,(s0)				# may cause nested xcpt.
		nop
		move	k0,s0

		lw	k1,FR_ENTRYHI(sp)
		nop
		mtc0	k1,CP0_ENTRYHI
		nop
		nop
		tlbp					# find faulting entry
		nop
		lw	k1,(k0)
		nop
		andi	k1,(_PAGE_PRESENT|_PAGE_WRITE)
		xori	k1,(_PAGE_PRESENT|_PAGE_WRITE)
		bnez	k1,nopage_tlbs
		nop
		/*
		 * Present and writable bits set: set accessed and dirty bits.
		 */
		lw	k1,(k0)				# delay slot
		nop
		ori	k1,k1,(_PAGE_ACCESSED|_PAGE_MODIFIED| \
			       _PAGE_VALID|_PAGE_DIRTY)
		sw	k1,(k0)
		/*
		 * Now reload the entry into the TLB
		 */
		mtc0	k1,CP0_ENTRYLO0
		nop
		nop
		nop				# for pipeline
		tlbwi
		nop				# for pipeline
#ifdef CONF_DEBUG_TLB
		la	k0,tlbs_lock
		li	k1,0
		sw	k1,(k0)
#endif
		j	return
		nop

		/*
		 * Page doesn't exist. Lots of work which is less important
		 * for speed needs to be done, so hand it all over to the
		 * kernel memory management routines.
		 */
nowrite_mod:
nopage_tlbs:

#ifdef CONFIG_TLB_SHUTDOWN
		/*
		 * Remove entry so we don't need to care later
		 */
		mfc0	k0,CP0_INDEX
		nop
#ifdef CONF_DEBUG_TLB
		bgez	k0,2f
		nop
		/*
		 * We got a tlbs exception but found no matching entry in
		 * the tlb.  This should never happen.  Paranoia makes us
		 * check it, though.
		 */
		.set noat
/*		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp) */
		jal	show_regs
		move	a0,sp
		.set	at
		la	a1,FR_BADVADDR(sp)
		lw	a1,(a1)
		nop
		PRINT("c0_badvaddr == %08lx\n")
		nop
		mfc0	a1,CP0_INDEX
		nop
		PRINT("c0_index    == %08x\n")
		nop
		la	a1,FR_ENTRYHI(sp)
		lw	a1,(a1)
		nop
		PRINT("c0_entryhi  == %08x\n")
		nop
		jal	dump_tlb_nonwired
		nop
		la	a0,FR_BADVADDR(sp)
		lw	a0,(a0)
		jal	dump_list_current
		nop
		
		.set	noat
		STI
		.set	at
		PANIC("Tlbs or tlbm exception with no matching entry in tlb")
1:		j	1b
		nop
2:
#endif /* CONF_DEBUG_TLB */
		lui	k1,0xa000
		sll	k0,4
		or	k0,k1
		xor	k0,k1
		or	k0,k1			# make it a KSEG1 address
		mtc0	k0,CP0_ENTRYHI
		nop
		mtc0	zero,CP0_ENTRYLO0
		nop
		nop
		nop
		tlbwi
		nop
#endif /* CONFIG_TLB_SHUTDOWN */

#ifdef CONF_DEBUG_TLB
		la	k0,tlbs_lock
		li	k1,0
		sw	k1,(k0)
#endif
		.set	noat
/*		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp) */
		lw	a2,FR_BADVADDR(sp)
		li	t1,-1
		sw	t1,FR_ORIG_REG2(sp)	# not a sys call
		nop
		STI
		.set	at
		/*
		 * a0 (struct pt_regs *) regs
		 * a1 (unsigned long)    1 for write access
		 * a2 (unsigned long)    faulting virtual address
		 */
		move	a0,sp
		jal	do_page_fault
		li	a1,1				# delay slot
		j	ret_from_sys_call
		nop					# delay slot
		END(handle_tlbs)

		.align	5
		NESTED(handle_mod, FR_SIZE, sp)
		.set	noat
		/*
		 * Two possible cases:
		 * 1) Page is writable but not dirty -> set dirty and return
		 * 2) Page is not writable -> call C handler
		 */
		/* used to be dmfc0 */

		SAVE_ALL
		REG_S	sp,FR_ORIG_REG2(sp)

		mfc0	s0,CP0_BADVADDR
		nop

		srl	s0,12
		sll	s0,2
		lui	k1,58368
		addu	s0,k1
		lw	k1,(s0)
		nop
		move	k0,s0
		nop

		lw	k1,FR_ENTRYHI(sp)
		nop
		mtc0	k1,CP0_ENTRYHI
		nop
		tlbp
		nop
		lw	k1,(k0)
		nop
		andi	k1,_PAGE_WRITE
		beqz	k1,nowrite_mod
		nop
		/*
		 * Present and writable bits set: set accessed and dirty bits.
		 */
		lw	k1,(k0)				# delay slot
		nop
		ori	k1,(_PAGE_ACCESSED|_PAGE_DIRTY)
		sw	k1,(k0)
		/*
		 * Now reload the entry into the tlb
		 */
		lw	k0,(k0)
		nop
		mtc0	k0,CP0_ENTRYLO0
		nop
		nop				# for pipeline
		nop
		tlbwi
		nop				# for pipeline
		j	return
		nop
		END(handle_mod)
		.set	at

		.set	reorder
		LEAF(tlbflush)

		.set	noreorder

		mfc0	t3,CP0_STATUS			# disable interrupts...
		nop
		ori	t4,t3,1
		xori	t4,1
		mtc0	t4,CP0_STATUS
                lw      t1,mips_tlb_entries /* mips_tlb_enbtries is set */
			                    /* by bi_EarlySnarf()       */
		mfc0	t0,CP0_ENTRYHI
		nop
		mtc0	zero,CP0_ENTRYLO0
		sll	t1,t1,8
		li	t2,KSEG1
		li	t5,(7<<8)	    /* R3000 has 8 wired entries */
1:
		subu	t1,(1<<8)
		beq	t1,t5,2f	    /* preserve wired entries    */

		sll	t6,t1,4
		addu	t6,t2
		mtc0	t6,CP0_ENTRYHI
		nop
		mtc0	t1,CP0_INDEX
		nop
		nop
		nop
		tlbwi
		nop
		b	1b
		nop
2:

		mtc0	t0,CP0_ENTRYHI
		nop
		mtc0	t3,CP0_STATUS
		nop
		jr	ra
		nop
		END(tlbflush)

/*
 * Flush a single entry from the TLB
 *
 * Parameters: a0 - unsigned long address
 */
		.set	noreorder
		LEAF(tlbflush_page)
		/*
		 * Step 1: Wipe out old TLB information.  Not shure if
		 * we really need that step; call it paranoia ...
		 * In order to do that we need to disable interrupts.
		 */
		mfc0	t0,CP0_STATUS		# interrupts off
		nop
		ori	t1,t0,1
		xori	t1,1
		mtc0	t1,CP0_STATUS
		li	t3,TLBMAP		# then wait 3 cycles
		ori	t1,a0,0xfff		# mask off low 12 bits
		xori	t1,0xfff
		mfc0	t2,CP0_ENTRYHI		# copy ASID into address
		nop
		andi	t2,0xfc0		# ASID in bits 11-6
		or	t2,t1
		mtc0	t2,CP0_ENTRYHI
/* FIXME:
	shouldn't we save ENTRYHI before trashing it ?
*/

		srl	t4,a0,12		# wait again three cycles
		sll	t4,t4,PTRLOG
		mtc0	zero,CP0_ENTRYLO0
		nop
		tlbp				# now query the TLB
		addu	t3,t4			# wait another three cycles
		ori	t3,0xffff
		xori	t3,0xffff
		mfc0	t1,CP0_INDEX
		nop
		blez	t1,1f			# No old entry?
		nop				# delay slot
		li	t5, KSEG1
		sll	t1,4
		addu	t5,t1
		mtc0	t5,CP0_ENTRYHI
		nop
		nop
		tlbwi
		/*
		 * But there still might be a entry for the pgd ...
		 */
1:		mtc0	t3,CP0_ENTRYHI
		nop				# wait 3 cycles
		nop
		nop
		tlbp				# TLB lookup
		nop
		nop
		mfc0	t1,CP0_INDEX		# wait 3 cycles
		nop
		blez	t1,1f			# No old entry?
		nop
		li	t5, KSEG1
		sll	t1,4
		addu	t5,t1
		mtc0	t5,CP0_ENTRYHI
		nop
		nop		
		tlbwi				# gotcha ...
		nop
		nop
		nop

1:	
		mtc0	t0,CP0_STATUS
		nop
		jr	ra
		nop

		END(tlbflush_page)

		.set	noreorder
		LEAF(tlbload)
		/* 
			address in a0 
			pte in a1
		*/

		mfc0	t1,CP0_STATUS
		nop
		ori	t0,t1,1
		xori	t0,1
		mtc0	t0,CP0_STATUS
		nop
		mfc0	t0,CP0_ENTRYHI
		nop
		ori	a0,0xfff
		xori	a0,0xfff
		andi	t2,t0,0xfc0
		or	a0,t2
		mtc0	a0,CP0_ENTRYHI
		nop
		nop
		mtc0	a1,CP0_ENTRYLO0

		la	t2, tlb_softIndex
		lw	t3,(t2)
		nop
		mtc0	t3, CP0_INDEX
		nop
		addu	t3,(1<<8)
		andi	t2,t3,(63<<8)
		bnez	t2, 1f
		nop
		li	t3,(8<<8)
1:		
		la	t2, tlb_softIndex
		sw	t3,(t2)


		nop
		nop
		nop
		tlbwi
		nop
		nop
		mtc0	t0,CP0_ENTRYHI
		nop
		mtc0	t1,CP0_STATUS

		jr ra
		nop
	
		END(tlbload)



/*
 * Code necessary to switch tasks on an Linux/MIPS machine.
 * FIXME: We don't need to disable interrupts anymore.
 */
		.align	5
		LEAF(resume)
		/*
		 * Current task's task_struct
		 */
		lui	t5,%hi(current)
		lw	t0,%lo(current)(t5)

		/*
		 * Save status register
		 */
		mfc0	t1,CP0_STATUS
		addu	t0,a1				# Add tss offset
		sw	t1,TOFF_CP0_STATUS(t0)

/*
		li	t2,ST0_CU0
		and	t2,t1
		beqz	t2,1f
		nop
		sw	sp,TOFF_KSP(t0)
1:
*/
		/*
		 * Disable interrupts
		 */
#ifndef __R3000__
		ori	t2,t1,0x1f
		xori	t2,0x1e
#else
		ori	t2,t1,1
		xori	t2,1
#endif
		mtc0	t2,CP0_STATUS

		/*
		 * Save non-scratch registers
		 * All other registers have been saved on the kernel stack
		 */
		sw	s0,TOFF_REG16(t0)
		sw	s1,TOFF_REG17(t0)
		sw	s2,TOFF_REG18(t0)
		sw	s3,TOFF_REG19(t0)
		sw	s4,TOFF_REG20(t0)
		sw	s5,TOFF_REG21(t0)
		sw	s6,TOFF_REG22(t0)
		sw	s7,TOFF_REG23(t0)
		sw	gp,TOFF_REG28(t0)
		sw	sp,TOFF_REG29(t0)
		sw	fp,TOFF_REG30(t0)

		/*
		 * Save floating point state
		 */
		sll	t2,t1,2
		bgez	t2,2f
		sw	ra,TOFF_REG31(t0)		# delay slot
		sll	t2,t1,5
		bgez	t2,1f
		swc1	$f0,(TOFF_FPU+0)(t0)		# delay slot
		/*
		 * Store the 16 odd double precision registers
		 */
		swc1	$f1,(TOFF_FPU+8)(t0)
		swc1	$f3,(TOFF_FPU+24)(t0)
		swc1	$f5,(TOFF_FPU+40)(t0)
		swc1	$f7,(TOFF_FPU+56)(t0)
		swc1	$f9,(TOFF_FPU+72)(t0)
		swc1	$f11,(TOFF_FPU+88)(t0)
		swc1	$f13,(TOFF_FPU+104)(t0)
		swc1	$f15,(TOFF_FPU+120)(t0)
		swc1	$f17,(TOFF_FPU+136)(t0)
		swc1	$f19,(TOFF_FPU+152)(t0)
		swc1	$f21,(TOFF_FPU+168)(t0)
		swc1	$f23,(TOFF_FPU+184)(t0)
		swc1	$f25,(TOFF_FPU+200)(t0)
		swc1	$f27,(TOFF_FPU+216)(t0)
		swc1	$f29,(TOFF_FPU+232)(t0)
		swc1	$f31,(TOFF_FPU+248)(t0)

		/*
		 * Store the 16 even double precision registers
		 */
1:		cfc1	t1,fcr31
		swc1	$f2,(TOFF_FPU+16)(t0)
		swc1	$f4,(TOFF_FPU+32)(t0)
		swc1	$f6,(TOFF_FPU+48)(t0)
		swc1	$f8,(TOFF_FPU+64)(t0)
		swc1	$f10,(TOFF_FPU+80)(t0)
		swc1	$f12,(TOFF_FPU+96)(t0)
		swc1	$f14,(TOFF_FPU+112)(t0)
		swc1	$f16,(TOFF_FPU+128)(t0)
		swc1	$f18,(TOFF_FPU+144)(t0)
		swc1	$f20,(TOFF_FPU+160)(t0)
		swc1	$f22,(TOFF_FPU+176)(t0)
		swc1	$f24,(TOFF_FPU+192)(t0)
		swc1	$f26,(TOFF_FPU+208)(t0)
		swc1	$f28,(TOFF_FPU+224)(t0)
		swc1	$f30,(TOFF_FPU+240)(t0)
		sw	t1,(TOFF_FPU+256)(t0)

		/*
		 * Switch current task
		 */
2:		sw	a0,%lo(current)(t5)
		addu	a0,a1			# Add tss offset

		/*
		 * Switch address space
		 */

		/*
		 * (Choose new ASID for process)
		 * This isn't really required, but would speed up
		 * context switching.
		 */

		/*
		 * Switch the root pointer
		 */
		lw	t0,TOFF_PG_DIR(a0)	# get PFN
		li	t1,TLB_ROOT
		mtc0	t1,CP0_ENTRYHI
		nop
		mtc0	zero,CP0_INDEX
		ori	t0,MODE_ALIAS		# want cachable, dirty, valid
		mtc0	t0,CP0_ENTRYLO0
		nop
		nop
		nop
		tlbwi 					# delay slot
		nop

		/*
		 * Flush tlb
		 * (probably not needed, doesn't clobber a0-a3)
		 */
		jal	tlbflush
		nop

		lw	a2,TOFF_CP0_STATUS(a0)
		nop

		/*
		 * Restore fpu state:
		 *  - cp0 status register bits
		 *  - fp gp registers
		 *  - cp1 status/control register
		 */
		ori	t1,a2,1				# pipeline magic
		xori	t1,1
		mtc0	t1,CP0_STATUS
		sll	t0,a2,2
		bgez	t0,2f
		sll	t0,a2,5				# delay slot
		bgez	t0,1f
		nop
		lwc1	$f0,(TOFF_FPU+0)(a0)		# delay slot
		/*
		 * Restore the 16 odd double precision registers only
		 * when enabled in the cp0 status register.
		 */
		lwc1	$f1,(TOFF_FPU+8)(a0)
		lwc1	$f3,(TOFF_FPU+24)(a0)
		lwc1	$f5,(TOFF_FPU+40)(a0)
		lwc1	$f7,(TOFF_FPU+56)(a0)
		lwc1	$f9,(TOFF_FPU+72)(a0)
		lwc1	$f11,(TOFF_FPU+88)(a0)
		lwc1	$f13,(TOFF_FPU+104)(a0)
		lwc1	$f15,(TOFF_FPU+120)(a0)
		lwc1	$f17,(TOFF_FPU+136)(a0)
		lwc1	$f19,(TOFF_FPU+152)(a0)
		lwc1	$f21,(TOFF_FPU+168)(a0)
		lwc1	$f23,(TOFF_FPU+184)(a0)
		lwc1	$f25,(TOFF_FPU+200)(a0)
		lwc1	$f27,(TOFF_FPU+216)(a0)
		lwc1	$f29,(TOFF_FPU+232)(a0)
		lwc1	$f31,(TOFF_FPU+248)(a0)

		/*
		 * Restore the 16 even double precision registers
		 * when cp1 was enabled in the cp0 status register.
		 */
1:		lw	t0,(TOFF_FPU+256)(a0)
		lwc1	$f2,(TOFF_FPU+16)(a0)
		lwc1	$f4,(TOFF_FPU+32)(a0)
		lwc1	$f6,(TOFF_FPU+48)(a0)
		lwc1	$f8,(TOFF_FPU+64)(a0)
		lwc1	$f10,(TOFF_FPU+80)(a0)
		lwc1	$f12,(TOFF_FPU+96)(a0)
		lwc1	$f14,(TOFF_FPU+112)(a0)
		lwc1	$f16,(TOFF_FPU+128)(a0)
		lwc1	$f18,(TOFF_FPU+144)(a0)
		lwc1	$f20,(TOFF_FPU+160)(a0)
		lwc1	$f22,(TOFF_FPU+176)(a0)
		lwc1	$f24,(TOFF_FPU+192)(a0)
		lwc1	$f26,(TOFF_FPU+208)(a0)
		lwc1	$f28,(TOFF_FPU+224)(a0)
		lwc1	$f30,(TOFF_FPU+240)(a0)
		ctc1	t0,fcr31

		/*
		 * Restore non-scratch registers
		 */
2:		lw	s0,TOFF_REG16(a0)
		lw	s1,TOFF_REG17(a0)
		lw	s2,TOFF_REG18(a0)
		lw	s3,TOFF_REG19(a0)
		lw	s4,TOFF_REG20(a0)
		lw	s5,TOFF_REG21(a0)
		lw	s6,TOFF_REG22(a0)
		lw	s7,TOFF_REG23(a0)
		lw	gp,TOFF_REG28(a0)
		lw	sp,TOFF_REG29(a0)
		lw	fp,TOFF_REG30(a0)
		lw	ra,TOFF_REG31(a0)

		/*
		 * Restore status register
		 */
		lw	t0,TOFF_KSP(a0)
		nop
		sw	t0,kernelsp

		mtc0	a2,CP0_STATUS			# delay slot
		jr	ra
		nop
		END(resume)

		/*
		 * Load a new root pointer into the tlb
		 */
		.set	noreorder
		LEAF(load_pgd)
		/*
		 * Switch the root pointer
		 */
		mfc0	t0,CP0_STATUS
		nop
		ori	t1,t0,1
		xori	t1,1
		mtc0	t1,CP0_STATUS

		ori	a0,MODE_ALIAS
		li	t1,TLB_ROOT
		mtc0	t1,CP0_ENTRYHI
		nop
		mtc0	zero,CP0_INDEX
		nop
		mtc0	a0,CP0_ENTRYLO0
		nop
		nop
		nop
		tlbwi
		nop
		nop
		mtc0	t0,CP0_STATUS
		nop
		jr	ra
		nop
		END(load_pgd)

/*
 * Some bits in the config register
 */
#define CONFIG_DB       (1<<4)
#define CONFIG_IB       (1<<5)

/*
 * Flush instruction/data caches - FIXME: Don't know how to do this on R[236]000!
 * (Actually most of this flushing stuff isn't needed for the R2000/R3000/R6000
 *  since these CPUs have physical indexed caches unlike R4000 and better
 *  which have virtual indexed caches.)
 *
 * Parameters: a0 - starting address to flush
 *             a1 - size of area to be flushed
 *             a2 - which caches to be flushed
 *
 * FIXME:      - ignores parameters in a0/a1
 *             - doesn't know about second level caches

 */
		.set	noreorder
		LEAF(mips1_cacheflush)

done:		j	cache_flush
		nop
		END(mips1_cacheflush)

/*
 * Invalidate virtual addresses. - FIXME: Don't know how on R[236]000 yet!
 * (Flushing is relativly expensive; it isn't required at all if a
 * particular machines' chipset keeps the external cache in a state that is
 * consistent with memory -- Ralf)
 *
 * Parameters: a0 - starting address to flush
 *             a1 - size of area to be flushed
 *
 * FIXME:      - ignores parameters in a0/a1
 *             - doesn't know about second level caches
 */
		.set	noreorder
		LEAF(fd_cacheflush)
		jr	ra
		nop
		END(fd_cacheflush)

/*
 * do_syscall calls the function in a1 with upto 7 arguments.  If over
 * four arguments are being requested, the additional arguments will
 * be copied from the user stack pointed to by a0->reg29.
 *
 * a0 (struct pt_regs *)  pointer to user registers
 * a1 (syscall_t)         pointer to syscall to do
 * a2 (int)               number of arguments to syscall
 */
		.set	reorder
		.text
NESTED(do_syscalls, 32, sp)
		subu	sp,32
		sw	ra,28(sp)
		sll	a2,a2,PTRLOG
		lw	t1,dst(a2)
		move	t2,a1
		lw	t0,FR_REG29(a0)		# get old user stack pointer
		jalr	t1

7:		lw	t1,24(t0)		# parameter #7 from usp
		sw	t1,24(sp)
6:		lw	t1,20(t0)		# parameter #6 from usp
		sw	t1,20(sp)
5:		lw	t1,16(t0)		# parameter #5 from usp
		sw	t1,16(sp)
4:		lw	a3,FR_REG7(a0)		# 4 args
3:		lw	a2,FR_REG6(a0)		# 3 args
2:		lw	a1,FR_REG5(a0)		# 2 args
1:		lw	a0,FR_REG4(a0)		# delay slot
		jalr	t2			# 1 args
		lw	ra,28(sp)
		addiu	sp,32
		jr	ra
0:		jalr	t2			# 0 args, just pass a0
		lw	ra,28(sp)
		addiu	sp,32
		jr	ra
		END(do_syscalls)

		.rdata
		.align	PTRLOG
dst:		PTR	0b, 1b, 2b, 3b, 4b, 5b, 6b, 7b

		.section	__ex_table,"a"
		PTR	7b,bad_stack
		PTR	6b,bad_stack
		PTR	5b,bad_stack

		.data

		EXPORT(tlbl_lock)
		.word	0

tlbs_lock:
		.word	0

		EXPORT(tlb_softIndex)
		.word	0
