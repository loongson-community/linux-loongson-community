/* $Id$
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
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/processor.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>

	.text

EXPORT(stext)					# used for profiling
EXPORT(_stext)

NESTED(kernel_entry, 16, sp)			# kernel entry point
	CLI					# disable interrupts

	la	$28, init_task_union		# init current pointer
	daddiu	t0, $28, KERNEL_STACK_SIZE-32
	sd	t0, kernelsp
	dsubu	sp, t0, 4*SZREG			# init stack pointer

	jal	start_kernel
1:	b	1b				# just in case ...
	END(kernel_entry)

	.comm	kernelsp, 8, 8			# current stackpointer

#define PAGE_SIZE	0x1000

	.macro	page name
	.globl	\name
	.org	. + PAGE_SIZE
\name:	.size	\name, PAGE_SIZE
	.type	\name, @object
	.endm

	.data

	page	swapper_pg_dir
	page	empty_bad_page
	page	empty_bad_page_table
	page	invalid_pte_table

	.align	13
