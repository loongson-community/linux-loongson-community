/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 Waldorf Electronics
 * Written by Ralf Baechle and Andreas Busse
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2003 Ralf Baechle
 * Copyright (C) 1996 Paul M. Antoine
 * Modified for DECStation and hence R3000 support by Paul M. Antoine
 * Further modifications by David S. Miller and Harald Koerfgen
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Kevin Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/threads.h>

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

	/*
	 * Reserved space for exception handlers.
	 * Necessary for machines which link their kernels at KSEG0.
	 */
	.fill	0x400

EXPORT(stext)					# used for profiling
EXPORT(_stext)

	__INIT

NESTED(kernel_entry, 16, sp)			# kernel entry point
	PTR_LA		$28, init_thread_union
	PTR_ADDIU	sp, $28, _THREAD_SIZE - 32
	PTR_S		sp, kernelsp
	PTR_SUBU	sp, 4 * SZREG		# init stack pointer

	/*
	 * The firmware/bootloader passes argc/argp/envp
	 * to us as arguments.  But clear bss first because
	 * the romvec and other important info is stored there
	 * by prom_init().
	 */
	PTR_LA		t0, __bss_start
	LONG_S		zero, (t0)
	PTR_LA		t1, __bss_stop - LONGSIZE
1:
	PTR_ADDIU	t0, LONGSIZE
	LONG_S		zero, (t0)
	bne		t0, t1, 1b

	jal		init_arch
	END(kernel_entry)

#ifdef CONFIG_SMP
/*
 * SMP slave cpus entry point.  Board specific code for bootstrap calls this
 * function after setting up the stack and gp registers.
 */
NESTED(smp_bootstrap, 16, sp)
	CLI
	mfc0	t0, CP0_STATUS
	li	t1, ~(ST0_CU1|ST0_CU2|ST0_CU3|ST0_KX|ST0_SX)
	and	t0, t1
	or	t0, ST0_CU0
	mtc0	t0, CP0_STATUS
	jal	start_secondary
	END(smp_bootstrap)
#endif /* CONFIG_SMP */

	__FINIT

	.comm	kernelsp,    NR_CPUS * 8, 8
	.comm	pgd_current, NR_CPUS * 8, 8

	.macro	page name, order=0
	.globl	\name
\name:	.size	\name, (_PAGE_SIZE << \order)
	.org	. + (_PAGE_SIZE << \order)
	.type	\name, @object
	.endm

	.data
	.align	PAGE_SHIFT

	page	swapper_pg_dir, _PGD_ORDER
	page	empty_bad_page, 0
	page	empty_bad_page_table, 0
	page	invalid_pte_table, 0
