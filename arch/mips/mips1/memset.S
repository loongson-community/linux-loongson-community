/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 *
 * Generic memset for all MIPS CPUs.
 * This is time critical.  Hear it crying "optimize me" ...
 */
#include <asm/asm.h>
#include <asm/regdef.h>

LEAF(__generic_memset_b)
__generic_memset_dw	=	__generic_memset_b
		.set	noreorder
		beqz		a2,2f
		LONG_ADDU	a3,a0,a2
		.set	reorder
		LONG_SUBU	a3,1
1:		sb		a1,(a0)
		.set	noreorder
		bne		a0,a3,1b
		LONG_ADDIU	a0,1
		.set	reorder
2:		jr		ra
		END(__generic_memset_b)
