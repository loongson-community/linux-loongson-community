/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 Silicon Graphics, Inc.
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 */
#include <linux/threads.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/pgtable.h>
#include <asm/stackframe.h>

	.data
	.comm	pgd_current, NR_CPUS * 8, 8

	/*
	 * After this macro runs we have a pointer to the pte of the address
	 * that caused the fault in in PTR.
	 */
	.macro	LOAD_PTE, ptr, tmp
#ifdef CONFIG_SMP
	mfc0	\tmp, CP0_CONTEXT
	dla	\ptr, pgd_current
	dsrl	\tmp, 23
	daddu	\ptr, \tmp
#else
	dla	\ptr, pgd_current
#endif
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)
	dsrl	\tmp, 28			# get pgd offset
	andi	\tmp, 0x1ff8
	daddu	\ptr, \tmp			# add in pgd offset
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)			# get pmd pointer
	dsrl	\tmp, 18			# get pmd offset
	andi	\tmp, 0x1ff8
	daddu	\ptr, \tmp			# add in pmd offset
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)			# get pte pointer
	dsrl	\tmp, 9
	andi	\tmp, 0xff8			# get pte offset
	daddu	\ptr, \tmp
	.endm

	.macro	LOAD_PTE2, ptr, tmp
#ifdef CONFIG_SMP
	mfc0	\tmp, CP0_CONTEXT
	dla	\ptr, pgd_current
	dsrl	\tmp, 23
	daddu	\ptr, \tmp
#else
	dla	\ptr, pgd_current
#endif
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)
	dsrl	\tmp, 28			# get pgd offset
	andi	\tmp, 0x1ff8
	daddu	\ptr, \tmp			# add in pgd offset
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)			# get pmd pointer
	dsrl	\tmp, 18			# get pmd offset
	andi	\tmp, 0x1ff8
	daddu	\ptr, \tmp			# add in pmd offset
	dmfc0	\tmp, CP0_XCONTEXT
	ld	\ptr, (\ptr)			# get pte pointer
	andi	\tmp, 0xff0			# get pte offset
	daddu	\ptr, \tmp
	.endm

	/*
	 * This places the even/odd pte pair in the page table at the pte
	 * entry pointed to by PTE into ENTRYLO0 and ENTRYLO1.
	 */
	.macro	PTE_RELOAD, pte0, pte1
	dsrl	\pte0, 6			# convert to entrylo0
	dmtc0	\pte0, CP0_ENTRYLO0		# load it
	dsrl	\pte1, 6			# convert to entrylo1
	dmtc0	\pte1, CP0_ENTRYLO1		# load it
	.endm

	.macro	DO_FAULT, write
	SAVE_ALL
	dmfc0	a2, CP0_BADVADDR
	STI
	.set	at
	move	a0, sp
	jal	do_page_fault
	 li	a1, \write
	j	ret_from_sys_call
	 nop
	.set	noat
	.endm

	/* Check if PTE is present, if not jump to LABEL.  */
	.macro	PTE_PRESENT, pte, ptr, label
	andi	\pte, (_PAGE_PRESENT | _PAGE_READ)
	xori	\pte, (_PAGE_PRESENT | _PAGE_READ)
	bnez	\pte, \label
	 nop
	ld	\pte, (\ptr)
	.endm

	/* Mark PTE as valid, and save in PTR.  */
	.macro	PTE_MAKEVALID, pte, ptr
	ori	\pte, (_PAGE_VALID | _PAGE_ACCESSED)
	sd	\pte, (\ptr)
	.endm

	/* Check if PTE is writable, if not jump to LABEL.  */
	.macro	PTE_WRITEABLE, pte, ptr, label
	andi	\pte, (_PAGE_PRESENT | _PAGE_WRITE)
	xori	\pte, (_PAGE_PRESENT | _PAGE_WRITE)
	bnez	\pte, \label
	 nop
	ld	\pte, (\ptr)
	.endm

	/* Mark PTE as writable, and save in PTR.  */
	.macro	PTE_MAKEWRITE, pte, ptr
	ori	\pte, (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | \
		       _PAGE_DIRTY)
	sd	\pte, (\ptr)
	.endm
	
	.text
	.set	noreorder
	.set	mips3

	.align	5
FEXPORT(except_vec0)
	.set	noat
1:	b	1b
	 nop

	/* TLB refill handler for the R10000.
	 * Attention:  We may only use 32 instructions.
	 */

	.align	5
FEXPORT(except_vec1_r10k)
	.set	noat
	LOAD_PTE2 k1 k0
	ld	k0, 0(k1)			# get even pte
	ld	k1, 8(k1)			# get odd pte
	PTE_RELOAD k0 k1
	nop
	tlbwr
	eret

	.align	5
FEXPORT(handle_tlbl)
	.set	noat
	LOAD_PTE k1 k0
	nop
	tlbp
	ld	k0, 0(k1)
	PTE_PRESENT k0 k1 nopage_tlbl
	PTE_MAKEVALID k0 k1
	ori	k1, 0xf
	xori	k1, 0xf
	ld	k0, 0(k1)
	ld	k1, 8(k1)
	PTE_RELOAD k0 k1
	nop
	tlbwi
	nop
	eret

nopage_tlbl:
	DO_FAULT 0

	.align	5
FEXPORT(handle_tlbs)
	.set	noat
	LOAD_PTE k1 k0
	nop
	tlbp
	ld	k0, 0(k1)
	PTE_WRITEABLE k0 k1 nopage_tlbs
	PTE_MAKEWRITE k0 k1
	ori	k1, 0xf
	xori	k1, 0xf
	ld	k0, 0(k1)
	ld	k1, 8(k1)
	PTE_RELOAD k0 k1
	nop
	tlbwi
	eret

nopage_tlbs:
	DO_FAULT 1

	.align	5
FEXPORT(handle_mod)
	.set	noat
	LOAD_PTE k1 k0
	nop
	tlbp
	ld	k0, 0(k1)
	PTE_WRITEABLE k0 k1 nowrite_mod
	PTE_MAKEWRITE k0 k1
	ori	k1, 0xf
	xori	k1, 0xf
	ld	k0, 0(k1)
	ld	k1, 8(k1)
	PTE_RELOAD k0 k1
	nop
	tlbwi
	eret

nowrite_mod:
	DO_FAULT 1
