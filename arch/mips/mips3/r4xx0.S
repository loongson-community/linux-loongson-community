/*
 * arch/mips/mips3/r4xx0.S
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle and Andreas Busse
 *
 * This file contains most of the R4xx0 specific routines.  Due to the
 * similarities this should hopefully also be fine for the R10000.  For
 * now we especially support the R10000 by not invalidating entries out of
 * the TLB before calling the C handlers.
 *
 * This code is evil magic. Read appendix F (coprocessor 0 hazards) of
 * all R4xx0 manuals and think about that MIPS means "Microprocessor without
 * Interlocked Pipeline Stages" before you even think about changing this code!
 *
 * CAVEATS: The R4000/R4400/R4600 manual say that the operation of a memory
 *          reference associated with a instruction immediately after a tlpb
 *          instruction is undefined.  It seems that the R4600 v2.0 also
 *          failes to handle the case where a tlbp instruction follows a
 *          (mapped???) memory reference.  In this case c0_index gets
 *          overwritten by some value which I suppose to be the entry
 *          mapping the associated instruction's memory reference.
 *
 * This needs a complete overhaul; it was written for a Linux kernel that
 * handled it's page tables the old i386 way ...
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
#include <asm/regdef.h>
#include <asm/stackframe.h>

/*
 * Use this to activate extra TLB error checking
 */
#define CONF_DEBUG_TLB

/*
 * Use this to activate extra TLB profiling code
 * (not implemented yet)
 */
#undef CONF_PROFILE_TLB

#ifdef __SMP__
#error "Fix this for SMP"
#else
#define current current_set
#endif

MODE_ALIAS	=	0x0016			# uncachable

		.text
		.set	noreorder

		.align	5
		NESTED(handle_tlbl, FR_SIZE, sp)
		.set	noat
		/*
		 * Check whether this is a refill or an invalid exception
		 *
		 * NOTE: Some MIPS manuals say that the R4x00 sets the
		 * BadVAddr only when EXL == 0. This is wrong - BadVAddr
		 * is being set for all Reload, Invalid and Modified
		 * exceptions.
		 */
		mfc0	k0,CP0_BADVADDR
		mfc0	k1,CP0_ENTRYHI
		ori	k0,0x1fff
		xori	k0,0x1fff
		andi	k1,0xff
		or	k0,k1
		mfc0	k1,CP0_ENTRYHI
		mtc0	k0,CP0_ENTRYHI
		nop					# for R4[04]00 pipeline
		nop
		nop
	nop
		tlbp
	nop
		nop					# for R4[04]00 pipeline
		nop
		mfc0	k0,CP0_INDEX
		bgez	k0,invalid_tlbl			# bad addr in c0_badvaddr
		mtc0	k1,CP0_ENTRYHI			# delay slot
		/*
		 * Damn... The next nop is required on my R4400PC V5.0, but
		 * I don't know why - at least there is no documented
		 * reason as for the others :-(
		 */
		nop

#ifdef CONF_DEBUG_TLB
		/*
		 * OK, this is a double fault. Let's see whether this is
		 * due to an invalid entry in the page_table.
		 */
		dmfc0	k0,CP0_BADVADDR
		srl	k0,12
		sll	k0,2
		lui	k1,%HI(TLBMAP)
		addu	k0,k1
		lw	k1,(k0)
		andi	k1,(_PAGE_PRESENT|_PAGE_ACCESSED)
		bnez	k1,reload_pgd_entries
		nop					# delay slot

		.set	noat
		SAVE_ALL
		.set	at
		PRINT("Double fault caused by invalid entries in pgd:\n")
		dmfc0	a1,CP0_BADVADDR
		PRINT("Double fault address     : %016Lx\n")
		dmfc0	a1,CP0_EPC
		PRINT("c0_epc                   : %016Lx\n")
		jal	show_regs
		move	a0,sp
		jal	dump_tlb_nonwired
		nop
		dmfc0	a0,CP0_BADVADDR
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
		dmfc0	k1,CP0_CONTEXT
//		mfc0	k1,CP0_CONTEXT
		dsra	k1,1
		lwu	k0,(k1)			# Never causes nested exception
		lwu	k1,4(k1)
		dsrl	k0,6			# Convert to EntryLo format
		dsrl	k1,6			# Convert to EntryLo format
		dmtc0	k0,CP0_ENTRYLO0
		dmtc0	k1,CP0_ENTRYLO1
		nop				# for R4[04]00 pipeline
		tlbwr
		nop				# for R4[04]00 pipeline
		nop
		nop
		/*
		 * We don't know whether the original access was read or
		 * write, so return and see what happens...
		 */
		eret

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
#ifdef CONFIG_TLB_SHUTDOWN
		/*
		 * Remove entry so we don't need to care later
		 * For sake of the R4000 V2.2 pipeline the tlbwi insn
		 * has been moved down. Moving it around is juggling with
		 * explosives...
		 */
		lui	k1,0x0008
		or	k0,k1
		dsll	k0,13
		dmtc0	k0,CP0_ENTRYHI
		dmtc0	zero,CP0_ENTRYLO0
		dmtc0	zero,CP0_ENTRYLO1
#endif
		/*
		 * Test present bit in entry
		 */
		dmfc0	k0,CP0_BADVADDR
		srl	k0,12
		sll	k0,2
#ifdef CONFIG_TLB_SHUTDOWN
		tlbwi						# do not move!
#endif
		lui	k1,%HI(TLBMAP)
		addu	k0,k1
		lw	k1,(k0)
		andi	k1,(_PAGE_PRESENT|_PAGE_READ)
		xori	k1,(_PAGE_PRESENT|_PAGE_READ)
		bnez	k1,nopage_tlbl
		/*
		 * Present and read bits are set -> set valid and accessed bits
		 */
		lw	k1,(k0)					# delay slot
		ori	k1,(_PAGE_VALID|_PAGE_ACCESSED)
		sw	k1,(k0)
		eret

		/*
		 * Page doesn't exist. Lots of work which is less important
		 * for speed needs to be done, so hand it all over to the
		 * kernel memory management routines.
		 */
nopage_tlbl:	SAVE_ALL
		dmfc0	a2,CP0_BADVADDR
		STI
		REG_S	sp,FR_ORIG_REG2(sp)
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
		dmfc0	k0,CP0_BADVADDR
		srl	k0,12
		sll	k0,2
		lui	k1,%HI(TLBMAP)
		addu	k0,k1
		lw	k1,(k0)
		nop
	nop
		tlbp					# find faulting entry
	nop
		andi	k1,(_PAGE_PRESENT|_PAGE_WRITE)
		xori	k1,(_PAGE_PRESENT|_PAGE_WRITE)
		bnez	k1,nopage_tlbs
		/*
		 * Present and writable bits set: set accessed and dirty bits.
		 */
		lw	k1,(k0)				# delay slot
		ori	k1,k1,(_PAGE_ACCESSED|_PAGE_MODIFIED| \
			       _PAGE_VALID|_PAGE_DIRTY)
		sw	k1,(k0)
		/*
		 * Now reload the entry into the TLB
		 */
		ori	k0,0x0004
		xori	k0,0x0004
		lw	k1,4(k0)
		lw	k0,(k0)
		srl	k1,6
		srl	k0,6
		dmtc0	k1,CP0_ENTRYLO1
		dmtc0	k0,CP0_ENTRYLO0
		nop				# for R4[04]00 pipeline
		tlbwi
		nop				# for R4[04]00 pipeline
		nop
		nop
		eret

		/*
		 * Page doesn't exist. Lots of work which is less important
		 * for speed needs to be done, so hand it all over to the
		 * kernel memory management routines.
		 */
nopage_tlbs:
nowrite_mod:
#ifdef CONFIG_TLB_SHUTDOWN
		/*
		 * Remove entry so we don't need to care later
		 */
		mfc0	k0,CP0_INDEX
#ifdef CONF_DEBUG_TLB
		bgez	k0,2f
		nop
		/*
		 * We got a tlbs exception but found no matching entry in
		 * the tlb.  This should never happen.  Paranoia makes us
		 * check it, though.
		 */
		SAVE_ALL
		jal	show_regs
		move	a0,sp
		.set	at
		dmfc0	a1,CP0_BADVADDR
		PRINT("c0_badvaddr == %016Lx\n")
		mfc0	a1,CP0_INDEX
		PRINT("c0_index    == %08x\n")
		dmfc0	a1,CP0_ENTRYHI
		PRINT("c0_entryhi  == %016Lx\n")
		.set	noat
		STI
		.set	at
		PANIC("Tlbs or tlbm exception with no matching entry in tlb")
1:		j	1b
		nop
2:
#endif /* CONF_DEBUG_TLB */
		lui	k1,0x0008
		or	k0,k1
		dsll	k0,13
		dmtc0	k0,CP0_ENTRYHI
		dmtc0	zero,CP0_ENTRYLO0
		dmtc0	zero,CP0_ENTRYLO1
		nop				# for R4[04]00 pipeline
		nop				# R4000 V2.2 requires 4 NOPs
		nop
		nop
		tlbwi
#endif /* CONFIG_TLB_SHUTDOWN */
		.set	noat
		SAVE_ALL
		dmfc0	a2,CP0_BADVADDR
		STI
		REG_S	sp,FR_ORIG_REG2(sp)
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
		dmfc0	k0,CP0_BADVADDR
		srl	k0,12
		sll	k0,2
		lui	k1,%HI(TLBMAP)
		addu	k0,k1
		lw	k1,(k0)
		nop
	nop
		tlbp					# find faulting entry
	nop
		andi	k1,_PAGE_WRITE
		beqz	k1,nowrite_mod
		/*
		 * Present and writable bits set: set accessed and dirty bits.
		 */
		lw	k1,(k0)				# delay slot
		ori	k1,(_PAGE_ACCESSED|_PAGE_DIRTY)
		sw	k1,(k0)
		/*
		 * Now reload the entry into the tlb
		 */
		ori	k0,0x0004
		xori	k0,0x0004
		lw	k1,4(k0)
		lw	k0,(k0)
		srl	k1,6
		srl	k0,6
		dmtc0	k1,CP0_ENTRYLO1
		dmtc0	k0,CP0_ENTRYLO0
		nop				# for R4[04]00 pipeline
		nop
		nop
		tlbwi
		nop				# for R4[04]00 pipeline
		nop
		nop
		eret
		END(handle_mod)
		.set	at

/*
 * Flush the complete TLB
 */
		.set	noreorder
		LEAF(tlbflush)
		li	t0,PM_4K
		mtc0	t0,CP0_PAGEMASK
		lw	t0,mips_tlb_entries /* mips_tlb_enbtries is set */
                                            /* by bi_EarlySnarf()       */
		dmtc0	zero,CP0_ENTRYLO0
		dmtc0	zero,CP0_ENTRYLO1
		mfc0	t2,CP0_WIRED
1:		subu	t0,1
		mtc0	t0,CP0_INDEX
		lui	t1,0x0008
		or	t1,t0,t1
		dsll	t1,13
		dmtc0	t1,CP0_ENTRYHI
		bne	t2,t0,1b
		tlbwi					# delay slot
		jr	ra
		nop					# delay slot
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
		li	t3,TLBMAP		# then wait 3 cycles
		ori	t1,a0,0xfff		# mask off low 12 bits
		xori	t1,0xfff
		mfc0	t2,CP0_ENTRYHI		# copy ASID into address
		andi	t2,0xff
		or	t2,t1
		mtc0	t2,CP0_ENTRYHI
		srl	t4,a0,12		# wait again three cycles
		sll	t4,t4,PTRLOG
		dmtc0	zero,CP0_ENTRYLO0
	nop
		tlbp				# now query the TLB
	nop
		addu	t3,t4			# wait another three cycles
		ori	t3,0xffff
		xori	t3,0xffff
		mfc0	t1,CP0_INDEX
		bltz	t1,1f			# No old entry?
		dmtc0	zero,CP0_ENTRYLO1
		or	t3,t1			# wait one cycle
		tlbwi
		/*
		 * But there still might be an entry for the pgd ...
		 */
1:		mtc0	t3,CP0_ENTRYHI
		nop				# wait 3 cycles
		nop
		nop
	nop
		tlbp				# TLB lookup
	nop
		nop
		nop
		mfc0	t1,CP0_INDEX		# wait 3 cycles
		bltz	t1,1f			# No old entry?
		nop		
		tlbwi				# gotcha ...
1:		jr	ra
		nop				# delay slot
		END(tlbflush_page)

/*
 * Code necessary to switch tasks on an Linux/MIPS machine.
 * FIXME: We don't need to disable interrupts anymore.
 * FIXME: Do some cache magic for faster loading/saving.
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
		 * Disable interrupts
		 */
		ori	t2,t1,0x1f
		xori	t2,0x1e
		mtc0	t2,CP0_STATUS

		/*
		 * Save non-scratch registers
		 * All other registers have been saved on the kernel stack
		 */
		sd	s0,TOFF_REG16(t0)
		sd	s1,TOFF_REG17(t0)
		sd	s2,TOFF_REG18(t0)
		sd	s3,TOFF_REG19(t0)
		sd	s4,TOFF_REG20(t0)
		sd	s5,TOFF_REG21(t0)
		sd	s6,TOFF_REG22(t0)
		sd	s7,TOFF_REG23(t0)
		sd	gp,TOFF_REG28(t0)
		sd	sp,TOFF_REG29(t0)
		sd	fp,TOFF_REG30(t0)

		/*
		 * Save floating point state
		 */
		sll	t2,t1,2
		bgez	t2,2f
		sd	ra,TOFF_REG31(t0)		# delay slot
		sll	t2,t1,5
		bgez	t2,1f
		sdc1	$f0,(TOFF_FPU+0)(t0)		# delay slot
		/*
		 * Store the 16 odd double precision registers
		 */
		sdc1	$f1,(TOFF_FPU+8)(t0)
		sdc1	$f3,(TOFF_FPU+24)(t0)
		sdc1	$f5,(TOFF_FPU+40)(t0)
		sdc1	$f7,(TOFF_FPU+56)(t0)
		sdc1	$f9,(TOFF_FPU+72)(t0)
		sdc1	$f11,(TOFF_FPU+88)(t0)
		sdc1	$f13,(TOFF_FPU+104)(t0)
		sdc1	$f15,(TOFF_FPU+120)(t0)
		sdc1	$f17,(TOFF_FPU+136)(t0)
		sdc1	$f19,(TOFF_FPU+152)(t0)
		sdc1	$f21,(TOFF_FPU+168)(t0)
		sdc1	$f23,(TOFF_FPU+184)(t0)
		sdc1	$f25,(TOFF_FPU+200)(t0)
		sdc1	$f27,(TOFF_FPU+216)(t0)
		sdc1	$f29,(TOFF_FPU+232)(t0)
		sdc1	$f31,(TOFF_FPU+248)(t0)

		/*
		 * Store the 16 even double precision registers
		 */
1:		cfc1	t1,fcr31
		sdc1	$f2,(TOFF_FPU+16)(t0)
		sdc1	$f4,(TOFF_FPU+32)(t0)
		sdc1	$f6,(TOFF_FPU+48)(t0)
		sdc1	$f8,(TOFF_FPU+64)(t0)
		sdc1	$f10,(TOFF_FPU+80)(t0)
		sdc1	$f12,(TOFF_FPU+96)(t0)
		sdc1	$f14,(TOFF_FPU+112)(t0)
		sdc1	$f16,(TOFF_FPU+128)(t0)
		sdc1	$f18,(TOFF_FPU+144)(t0)
		sdc1	$f20,(TOFF_FPU+160)(t0)
		sdc1	$f22,(TOFF_FPU+176)(t0)
		sdc1	$f24,(TOFF_FPU+192)(t0)
		sdc1	$f26,(TOFF_FPU+208)(t0)
		sdc1	$f28,(TOFF_FPU+224)(t0)
		sdc1	$f30,(TOFF_FPU+240)(t0)
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
		lw	t0,TOFF_PG_DIR(a0)
		li	t1,TLB_ROOT
		mtc0	t1,CP0_ENTRYHI
		mtc0	zero,CP0_INDEX
		srl	t0,6
		ori	t0,MODE_ALIAS
		mtc0	t0,CP0_ENTRYLO0
		mtc0	zero,CP0_ENTRYLO1
		lw	a2,TOFF_CP0_STATUS(a0)

		/*
		 * Flush tlb
		 * (probably not needed, doesn't clobber a0-a3)
		 */
		jal	tlbflush
		tlbwi 					# delay slot

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
		ldc1	$f0,(TOFF_FPU+0)(a0)		# delay slot
		/*
		 * Restore the 16 odd double precision registers only
		 * when enabled in the cp0 status register.
		 */
		ldc1	$f1,(TOFF_FPU+8)(a0)
		ldc1	$f3,(TOFF_FPU+24)(a0)
		ldc1	$f5,(TOFF_FPU+40)(a0)
		ldc1	$f7,(TOFF_FPU+56)(a0)
		ldc1	$f9,(TOFF_FPU+72)(a0)
		ldc1	$f11,(TOFF_FPU+88)(a0)
		ldc1	$f13,(TOFF_FPU+104)(a0)
		ldc1	$f15,(TOFF_FPU+120)(a0)
		ldc1	$f17,(TOFF_FPU+136)(a0)
		ldc1	$f19,(TOFF_FPU+152)(a0)
		ldc1	$f21,(TOFF_FPU+168)(a0)
		ldc1	$f23,(TOFF_FPU+184)(a0)
		ldc1	$f25,(TOFF_FPU+200)(a0)
		ldc1	$f27,(TOFF_FPU+216)(a0)
		ldc1	$f29,(TOFF_FPU+232)(a0)
		ldc1	$f31,(TOFF_FPU+248)(a0)

		/*
		 * Restore the 16 even double precision registers
		 * when cp1 was enabled in the cp0 status register.
		 */
1:		lw	t0,(TOFF_FPU+256)(a0)
		ldc1	$f2,(TOFF_FPU+16)(a0)
		ldc1	$f4,(TOFF_FPU+32)(a0)
		ldc1	$f6,(TOFF_FPU+48)(a0)
		ldc1	$f8,(TOFF_FPU+64)(a0)
		ldc1	$f10,(TOFF_FPU+80)(a0)
		ldc1	$f12,(TOFF_FPU+96)(a0)
		ldc1	$f14,(TOFF_FPU+112)(a0)
		ldc1	$f16,(TOFF_FPU+128)(a0)
		ldc1	$f18,(TOFF_FPU+144)(a0)
		ldc1	$f20,(TOFF_FPU+160)(a0)
		ldc1	$f22,(TOFF_FPU+176)(a0)
		ldc1	$f24,(TOFF_FPU+192)(a0)
		ldc1	$f26,(TOFF_FPU+208)(a0)
		ldc1	$f28,(TOFF_FPU+224)(a0)
		ldc1	$f30,(TOFF_FPU+240)(a0)
		ctc1	t0,fcr31

		/*
		 * Restore non-scratch registers
		 */
2:		ld	s0,TOFF_REG16(a0)
		ld	s1,TOFF_REG17(a0)
		ld	s2,TOFF_REG18(a0)
		ld	s3,TOFF_REG19(a0)
		ld	s4,TOFF_REG20(a0)
		ld	s5,TOFF_REG21(a0)
		ld	s6,TOFF_REG22(a0)
		ld	s7,TOFF_REG23(a0)
		ld	gp,TOFF_REG28(a0)
		ld	sp,TOFF_REG29(a0)
		ld	fp,TOFF_REG30(a0)
		ld	ra,TOFF_REG31(a0)

		/*
		 * Restore status register
		 */
		lw	t0,TOFF_KSP(a0)
		sw	t0,kernelsp

		jr	ra
		mtc0	a2,CP0_STATUS			# delay slot
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
		ori	t1,t0,1
		xori	t1,1
		mtc0	t1,CP0_STATUS
		srl	a0,6
		ori	a0,MODE_ALIAS
		li	t1,TLB_ROOT
		mtc0	t1,CP0_ENTRYHI
		mtc0	zero,CP0_INDEX
		mtc0	a0,CP0_ENTRYLO0
		mtc0	zero,CP0_ENTRYLO1
		mtc0	t0,CP0_STATUS
		jr	ra
		tlbwi 					# delay slot
		END(load_pgd)

/*
 * do_syscall calls the function in a1 with upto 7 arguments.  If over
 * four arguments are being requested, the additional arguments will
 * be copied from the user stack pointed to by a0->reg29.
 *
 * FIXME: This function will fail for syscalls with more than four
 *        arguments from kernelspace.
 *
 * a0 (struct pt_regs *)  pointer to user registers
 * a1 (syscall_t)         pointer to syscall to do
 * a2 (int)               number of arguments to syscall
 */
#if defined (__MIPSEB__) && defined (__mips64)
#define bo 4
#else
#define bo 0
#endif
		.set	reorder
		.text
NESTED(do_syscalls, 64, sp)
		LONG_SUBU sp,64
		REG_S	ra,56(sp)
		dsll	a2,a2,PTRLOG
		lw	t1,dst(a2)
		move	t2,a1
		REG_L	t0,FR_REG29(a0)		# get old user stack pointer
		jalr	t1

7:		lw	t1,24+bo(t0)		# parameter #7 from usp
		REG_S	t1,SZREG*6(sp)
6:		lw	t1,20+bo(t0)		# parameter #6 from usp
		REG_S	t1,SZREG*5(sp)
5:		lw	t1,16+bo(t0)		# parameter #5 from usp
		REG_S	t1,SZREG*4(sp)

4:		lw	a3,FR_REG7+bo(a0)	# 4 args
3:		lw	a2,FR_REG6+bo(a0)	# 3 args
2:		lw	a1,FR_REG5+bo(a0)	# 2 args
1:		lw	a0,FR_REG4+bo(a0)	# 1 args
		jalr	t2		
		ld	ra,56(sp)
		addiu	sp,64
		jr	ra
0:		jalr	t2			# 0 args, just pass a0
		ld	ra,56(sp)
		LONG_ADDIU sp,64
		jr	ra
		END(do_syscalls)

		.rdata
		.align	PTRLOG
dst:		PTR	0b, 1b, 2b, 3b, 4b, 5b, 6b, 7b

		.section	__ex_table,"a"
		PTR	7b,bad_stack
		PTR	6b,bad_stack
		PTR	5b,bad_stack
		.text

#ifdef CONFIG_REVERSE

/*
 * Same as above but for processes running with reverse byte order.
 * When exec(2)ing a file the system detects normal/reverse byteorder in
 * an executable and
 *   - sets the appropriate vector for syscalls and other low level
 *     routines that depend of userspace byteorder.
 *   - set the reverse byteorder flag in c0_status
 *   - marks the process as reverse in the process table.
 */
#define REG_SWAP(r,t)                           \
		.set	push;                   \
		.set	noat;                   \
		sll	$1,r,24;                \
		andi	t,r,0xff00;             \
		sll	t,$t,8;                 \
		or	$1,t;                   \
		srl	t,r,8;                  \
		andi	t,$t,0xff00;            \
		or	$1,t;                   \
		srl	r,r,24;                 \
		or	$1,r;                   \
		.set	pop

		.set	reorder
		.text
NESTED(do_syscalls_rev, 64, sp)
		LONG_SUBU sp,64
		REG_S	ra,56(sp)
		dsll	a2,a2,PTRLOG
		lw	t1,dst_rev(a2)
		move	t2,a1
		REG_L	t0,FR_REG29(a0)		# get old user stack pointer
		jalr	t1

7:		lw	t1,24+bo(t0)		# parameter #7 from usp
		REG_SWAP(t1,v0)			# convert byteorder
		REG_S	t1,SZREG*6(sp)
6:		lw	t1,20+bo(t0)		# parameter #6 from usp
		REG_SWAP(t1,v0)			# convert byteorder
		REG_S	t1,SZREG*5(sp)
5:		lw	t1,16+bo(t0)		# parameter #5 from usp
		REG_SWAP(t1,v0)			# convert byteorder
		REG_S	t1,SZREG*4(sp)

4:		lw	a3,FR_REG7+bo(a0)	# 4 args
3:		lw	a2,FR_REG6+bo(a0)	# 3 args
2:		lw	a1,FR_REG5+bo(a0)	# 2 args
1:		lw	a0,FR_REG4+bo(a0)	# 1 args
		jalr	t2		
		ld	ra,56(sp)
		addiu	sp,64
		jr	ra
0:		jalr	t2			# 0 args, just pass a0
		ld	ra,56(sp)
		LONG_ADDIU sp,64
		jr	ra
		END(do_syscalls)

		.rdata
		.align	PTRLOG
dst_rev:	PTR	0b, 1b, 2b, 3b, 4b, 5b, 6b, 7b

		.section	__ex_table,"a"
		PTR	7b,bad_stack
		PTR	6b,bad_stack
		PTR	5b,bad_stack
		.text

#endif /* CONFIG_REVERSE */
