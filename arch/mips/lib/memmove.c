/*
 * arch/mips/lib/memmove.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1994, 1995, 1996 by Ralf Baechle
 *
 * Less stupid implementation of memmove.
 */
#include <linux/string.h>

void __memmove(void *dest, const void *src, size_t n)
{
	if (!n)
		return;

	if (dest < src
	    || dest > src + n)
		/* Copy forward */
		__memcpy(dest, src, n);
	else
		/* Copy backwards */
		__asm__ __volatile__(
			".set\tnoreorder\n\t"
			".set\tnoat\n"
			"1:\tlbu\t$1,-1(%1)\n\t"
			"subu\t%1,1\n\t"
			"sb\t$1,-1(%0)\n\t"
			"subu\t%2,1\n\t"
			"bnez\t%2,1b\n\t"
			"subu\t%0,1\n\t"
			".set\tat\n\t"
			".set\treorder"
			: "=r" (dest), "=r" (src), "=r" (n)
			: "0" (dest + n), "1" (src + n), "2" (n)
			: "$1","memory" );
}
