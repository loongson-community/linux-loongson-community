/* $Id: head.S,v 1.6 2000/03/27 21:05:04 ulfc Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Head.S contains the MIPS exception handler and startup code.
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle and Andreas Busse
 * Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#define __ASSEMBLY__
#include <linux/config.h>
#include <linux/init.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/processor.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/pgtable.h>

	.text

EXPORT(stext)					# used for profiling
EXPORT(_stext)

	__INIT

NESTED(kernel_entry, 16, sp)			# kernel entry point

	ori	sp, 0xf				# align stack on 16 byte.
	xori	sp, 0xf

	jal	mapped_kernel_setup_tlb

#ifdef CONFIG_ARC64
	la	t0, 1f
	jr	t0
1:
#endif

	/* Note that all firmware passed argument registers still
	   have their values.  */
	jal	prom_init			# initialize firmware

	CLI					# disable interrupts

	mfc0	t0, CP0_STATUS
	/*
	 * On IP27, I am seeing the TS bit set when the
	 * kernel is loaded. Maybe because the kernel is
	 * in ckseg0 and not xkphys? Clear it anyway ...
	 */
	li	t1, ~(ST0_TS|ST0_CU1|ST0_CU2|ST0_CU3)
	and	t0, t1
	or	t0, (ST0_CU0|ST0_KX|ST0_SX|ST0_FR) # Bogosity: cu0 indicates kernel
	mtc0	t0, CP0_STATUS			   # thread in copy_thread.

	la	$28, init_task_union		# init current pointer
	daddiu	t0, $28, KERNEL_STACK_SIZE-32
	sd	t0, kernelsp
	dsubu	sp, t0, 4*SZREG			# init stack pointer
	move	t0, $28
#ifdef CONFIG_SMP
	mtc0	t0, CP0_WATCHLO
	dsrl32	t0, t0, 0
	mtc0	t0, CP0_WATCHHI
#endif
	jal	start_kernel
1:	b	1b				# just in case ...
	END(kernel_entry)

NESTED(mapped_kernel_setup_tlb, 16, sp)
#ifdef CONFIG_MAPPED_KERNEL
	/*
	 * This needs to read the nasid - assume 0 for now.
	 * Drop in 0xffffffffc0000000 in tlbhi, 0+DVG in tlblo_0,
	 * 0x10000+DVG in tlblo_1. Assumes sash will load kernel
	 * at physical 0x1c000 and virtual 0xa80000000001c000.
	 * (mips64/Makefile has a --change-address to do this).
	 */
	or	a0, zero, zero
	dli	t0, 0xffffffffc0000000
	dmtc0	t0, CP0_ENTRYHI
	li	t0, ((_PAGE_GLOBAL|_PAGE_VALID| _PAGE_DIRTY|_CACHE_CACHABLE_COW) >> 6)
	mtc0	t0, CP0_ENTRYLO0	# physaddr 0, DVG, cach exlwr
	mtc0	zero, CP0_ENTRYLO1	# 16M should be enough for text+data
	li	t0, 0x1ffe000		# MAPPED_KERN_TLBMASK, TLBPGMASK_16M
	mtc0	t0, CP0_PAGEMASK
	li	t0, 0			# KMAP_INX
	mtc0	t0, CP0_INDEX
	li	t0, 1
	mtc0	t0, CP0_WIRED
	tlbwi
#else
	mtc0	zero, CP0_WIRED
	tlbwi
#endif
	jr	ra
	END(mapped_kernel_setup_tlb)

NESTED(bootstrap, 16, sp)
	jal	mapped_kernel_setup_tlb
#ifdef CONFIG_ARC64
	la	t0, 1f
	jr	t0
1:
#endif
	CLI
	mfc0	t0, CP0_STATUS
	li	t1, ~(ST0_CU1|ST0_CU2|ST0_CU3)
	and	t0, t1
	or	t0, (ST0_CU0|ST0_KX|ST0_SX|ST0_FR) # Bogosity: cu0 indicates kernel
	mtc0	t0, CP0_STATUS			   # thread in copy_thread.
	jal	cboot
	END(bootstrap)

	__FINIT

	.comm	kernelsp, 8, 8			# current stackpointer

#define PAGE_SIZE	0x1000

	.macro	page name, order=0
	.globl	\name
	.org	. + (PAGE_SIZE << \order)
\name:	.size	\name, (PAGE_SIZE << \order)
	.type	\name, @object
	.endm

	.align	12
	.data

	page	swapper_pg_dir, 1
	page	invalid_pte_table, 1
	page	invalid_pmd_table, 1
	page	empty_bad_page_table, 1
	page	empty_bad_page
