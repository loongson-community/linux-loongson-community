/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 Silicon Graphics, Inc.
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 * Copyright (C) 2002  Maciej W. Rozycki
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/threads.h>

#include <asm/asm.h>
#include <asm/hazards.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/war.h>

#define _VMALLOC_START	0xc000000000000000

	.macro	GET_PGD, ptr
#ifdef CONFIG_SMP
	/*
	 * Fixme - this is b0rked for pgd_current outside of CKSEG0
	 */
	dmfc0	\ptr, CP0_CONTEXT
	dsra	\ptr, 23			# get pgd_current[cpu]
	ld	\ptr, (\ptr)
#else
	ld	\ptr, pgd_current
#endif
	.endm

	/*
	 * After this macro runs we have a pointer to the pte of the address
	 * that caused the fault in PTR.  Expects register containing the
	 * the pagetable root pointer as the ptr argument and c0_badvaddr
	 * passed as tmp argument.
	 */
	.macro	LOAD_PTE2, ptr, tmp
	dsrl	\tmp, (_PGDIR_SHIFT-3)		# get pgd offset in bytes
	andi	\tmp, ((_PTRS_PER_PGD - 1)<<3)
	daddu	\ptr, \tmp			# add in pgd offset
	dmfc0	\tmp, CP0_BADVADDR
	ld	\ptr, (\ptr)			# get pmd pointer
	dsrl	\tmp, (_PMD_SHIFT-3)		# get pmd offset in bytes
	andi	\tmp, ((_PTRS_PER_PMD - 1)<<3)
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

	__INIT

	/*
	 * TLB refill handlers for the R4000 and SB1.
	 * Attention:  We may only use 32 instructions / 128 bytes.
	 */
	.align  5
LEAF(except_vec1_r4k)
	.set    noat
	dla     k0, handle_vec1_r4k
	jr      k0
	 nop
END(except_vec1_r4k)

LEAF(except_vec1_sb1)
#if BCM1250_M3_WAR
	dmfc0	k0, CP0_BADVADDR
	dmfc0	k1, CP0_ENTRYHI
	xor	k0, k1
	dsrl	k0, k0, _PAGE_SHIFT+1
	bnez	k0, 1f
#endif
	.set    noat
	dla     k0, handle_vec1_r4k
	jr      k0
	 nop

1:	eret
	nop
END(except_vec1_sb1)

	__FINIT

	.align  5
LEAF(handle_vec1_r4k)
	.set    noat
	dmfc0	k0, CP0_BADVADDR
	bltz	k0, 9f

	 GET_PGD k1				# pointer to root of pgd
	LOAD_PTE2 k1 k0
	ld	k0, 0(k1)			# get even pte
	ld	k1, 8(k1)			# get odd pte
	PTE_RELOAD k0 k1
	mtc0_tlbw_hazard
	tlbwr
	tlbw_eret_hazard
	eret

9:						# handle the vmalloc range
	dli	k1, _VMALLOC_START
	dsubu	k0, k1
	dla	k1, swapper_pg_dir		# pointer to root of pgd
	LOAD_PTE2 k1 k0
	ld	k0, 0(k1)			# get even pte
	ld	k1, 8(k1)			# get odd pte
	PTE_RELOAD k0 k1
	mtc0_tlbw_hazard
	 tlbwr
	tlbw_eret_hazard
	eret
END(handle_vec1_r4k)
