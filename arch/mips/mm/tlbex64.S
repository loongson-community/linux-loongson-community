/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999 Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#include <linux/init.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/war.h>

	.macro	tlb_handler name writebit
	NESTED(__\name, PT_SIZE, sp)
#if BCM1250_M3_WAR
	dmfc0	k0, CP0_BADVADDR
	dmfc0	k1, CP0_ENTRYHI
	xor	k0, k1
	dsrl	k0, k0, PAGE_SHIFT + 1
	bnez	k0, 1f
#endif
	SAVE_ALL
	dmfc0	a2, CP0_BADVADDR
	KMODE
	li	a1, \writebit
	sd	a2, PT_BVADDR(sp)
	move	a0, sp
	jal	do_page_fault
#if BCM1250_M3_WAR
1:
#endif
	j	ret_from_exception
	END(__\name)
	.endm

	tlb_handler	xtlb_mod 1
	tlb_handler	xtlb_tlbl 0
	tlb_handler	xtlb_tlbs 1
