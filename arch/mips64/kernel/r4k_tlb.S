/* $Id: r4k_tlb.S,v 1.1 2000/05/25 19:33:16 ulfc Exp $
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
