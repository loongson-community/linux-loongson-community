/*
 * arch/mips/lib/bcopy.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1994, 1995, 1996 by Ralf Baechle
 *
 * bcopy() only exists here such that it doesn't get compiled into
 * lib/strings.o.  Though it's more efficient ...
 */
#include <linux/string.h>

char * bcopy(const char *src, char *dest, size_t count)
{
	__memcpy(dest, src, count);

	return dest;
}
