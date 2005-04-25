/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2005 Thiemo Seufer
 */
#include <linux/init.h>

#include <asm/addrspace.h>

/*
 * FUNC is executed in the uncached segment CKSEG1. This works only if
 * both code and stack live in CKSEG0. The stack handling works because
 * we don't handle stack arguments or more complex return values, so we
 * can avoid to share the same stack area between cached and uncached
 * mode.
 */
unsigned long __init run_uncached(void *func)
{
	register unsigned long sp __asm__("$sp");
	register unsigned long ret __asm__("$2");
	unsigned long usp = sp - CAC_BASE + UNCAC_BASE;
	unsigned long ufunc = func - CAC_BASE + UNCAC_BASE;

	__asm__ __volatile__ (
		"	move $16, $sp\n"
		"	move $sp, %1\n"
		"	jalr $ra, %2\n"
		"	move $sp, $16"
		: "=&r" (ret)
		: "r" (usp), "r" (ufunc)
		: "$16", "$31");

	return ret;
}
