/*
 * linux/include/asm-mips/mipsconfig.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 by Waldorf GMBH
 * written by Ralf Baechle
 *
 */
#ifndef _ASM_MIPS_MIPS_CONFIG_H
#define _ASM_MIPS_MIPS_CONFIG_H

/*
 * This is the virtual adress to which all ports are being mapped.
 */
#define PORT_BASE		0xe0000000
#define PORT_BASE_HIGH		0xe000

#define NUMBER_OF_TLB_ENTRIES	48

/*
 * Absolute address of the kernelstack is 0x80000280
 */
#define KERNEL_SP_HIGH		0x8000
#define KERNEL_SP_LOW		0x0280

#endif /* _ASM_MIPS_MIPS_CONFIG_H */
