/*
 *  linux/arch/mips/philips/nino/wbflush.c
 *
 *  Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Function to flush the write buffer on the Philips Nino.
 */
#include <linux/init.h>

void (*__wbflush) (void);

void nino_wbflush(void)
{
	/*
	 * The status of the writeback buffer is available
	 * via the Coprocessor 0 condition
	 */
	__asm__ __volatile__(
		".set\tpush\n\t"
		".set\tnoreorder\n\t"
		".set\tmips3\n\t"
		"sync\n\t"
		"nop\n\t"
		"1:\tbc0f\t1b\n\t"
		"nop\n\t"
		".set\tpop");
}

void __init wbflush_setup(void)
{
        __wbflush = nino_wbflush;
}
