/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 * Copyright (C) 2002 Ralf Baechle
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __ASM_SIBYTE_64BIT_H
#define __ASM_SIBYTE_64BIT_H

#include <linux/config.h>
#include <linux/types.h>

#ifdef CONFIG_MIPS32

#include <asm/system.h>

/*
 * This is annoying...we can't actually write the 64-bit IO register properly
 * without having access to 64-bit registers...  which doesn't work by default
 * in o32 format...grrr...
 */
static inline void out64(u64 val, unsigned long addr)
{
	u32 low, high;
	unsigned long flags;
	high = val >> 32;
	low = val & 0xffffffff;
	// save_flags(flags);
	__save_and_cli(flags);
	__asm__ __volatile__ (
		".set push\n"
		".set noreorder\n"
		".set noat\n"
		".set mips4\n"
		"   dsll32 $2, %1, 0   \n"
		"   dsll32 $1, %0, 0   \n"
		"   dsrl32 $2, $2, 0   \n"
		"   or     $1, $1, $2  \n"
		"   sd $1, (%2)\n"
		".set pop\n"
		::"r" (high), "r" (low), "r" (addr)
		:"$1", "$2");
	__restore_flags(flags);
}

static inline u64 in64(unsigned long addr)
{
	u32 low, high;
	unsigned long flags;
	__save_and_cli(flags);
	__asm__ __volatile__ (
		".set push\n"
		".set noreorder\n"
		".set noat     \n"
		".set mips4    \n"
		"  ld     %1, (%2)\n"
		"  dsra32 %0, %1, 0\n"
		"  sll    %1, %1, 0\n"
		".set pop\n"
		:"=r" (high), "=r" (low): "r" (addr));
	__restore_flags(flags);
	return (((u64)high) << 32) | low;
}

#endif /* CONFIG_MIPS32 */

#ifdef CONFIG_MIPS64

/*
 * These are provided so as to be able to use common
 * driver code for the 32-bit and 64-bit trees
 */
extern inline void out64(u64 val, unsigned long addr)
{
	*(volatile unsigned long *)addr = val;
}

extern inline u64 in64(unsigned long addr)
{
	return *(volatile unsigned long *)addr;
}

#endif /* CONFIG_MIPS64 */

/*
 * Avoid interrupt mucking, just adjust the address for 4-byte access.
 * Assume the addresses are 8-byte aligned.
 */

#ifdef __MIPSEB__
#define __CSR_32_ADJUST 4
#else
#define __CSR_32_ADJUST 0
#endif

#define csr_out32(v,a) (*(u32 *)((unsigned long)(a) + __CSR_32_ADJUST) = (v))
#define csr_in32(a)    (*(u32 *)((unsigned long)(a) + __CSR_32_ADJUST))

#endif /* __ASM_SIBYTE_64BIT_H */
