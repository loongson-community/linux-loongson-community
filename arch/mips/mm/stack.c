/*
 * Kernel stack allocation/deallocation
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 *
 * (This is _bad_ if the free page pool is fragmented ...)
 */
#include <linux/sched.h>
#include <linux/mm.h>

extern unsigned long alloc_kernel_stack(void)
{
	unsigned long stack;
	stack = __get_free_pages(GFP_KERNEL, 1, 0);

	return stack;
}

extern void free_kernel_stack(unsigned long stack)
{
	free_pages(stack, 1);
}
