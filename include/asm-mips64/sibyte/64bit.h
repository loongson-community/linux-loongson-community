/*
 * Copyright (C) 2001 Broadcom Corporation
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

#ifndef _ASM_SIBYTE_64BIT_H
#define _ASM_SIBYTE_64BIT_H

#include <linux/types.h>

/* These are provided so as to be able to use common
   driver code for the 32-bit and 64-bit trees */

extern inline void out64(u64 val, unsigned long addr)
{
	*(volatile unsigned long *)addr = val;
}

extern inline u64 in64(unsigned long addr)
{
	return *(volatile unsigned long *)addr;
}

#endif /* _ASM_SIBYTE_64BIT_H */
