/*
 * include/asm-mips/bitops.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1994 by Ralf Baechle
 */
#ifndef _ASM_MIPS_BITOPS_H
#define _ASM_MIPS_BITOPS_H

#include <asm/mipsregs.h>

extern inline int set_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw|mask));

	return retval;
}

extern inline int clear_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw & ~mask));

	return retval;
}

extern inline int change_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw ^ mask));

	return retval;
}

extern inline int test_bit(int nr, void *addr)
{
	int	mask;
	int	*a;

	a = addr;
	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *a) != 0);
}


/*
 * The above written is not true for the bitfield functions.
 */
static inline int find_first_zero_bit (void *addr, unsigned size)
{
	int res;

	if (!size)
		return 0;

	__asm__(".set\tnoreorder\n\t"
		".set\tnoat\n"
		"1:\tsubu\t$1,%2,%0\n\t"
		"blez\t$1,2f\n\t"
		"lw\t$1,(%4)\n\t"
		"addiu\t%4,%4,4\n\t"
		"beql\t%1,$1,1b\n\t"
		"addiu\t%0,%0,32\n\t"
		"li\t%1,1\n"
		"1:\tand\t%4,$1,%1\n\t"
		"beq\t$0,%4,2f\n\t"
		"sll\t%1,%1,1\n\t"
		"bne\t$0,%1,1b\n\t"
		"add\t%0,%0,1\n\t"
		".set\tat\n\t"
		".set\treorder\n"
		"2:"
		: "=d" (res)
		: "d" ((unsigned int) 0xffffffff),
		  "d" (size),
		  "0" ((signed int) 0),
		  "d" (addr)
		: "$1");

	return res;
}

static inline int find_next_zero_bit (void * addr, int size, int offset)
{
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;
	
	if (bit) {
		/*
		 * Look for zero in first byte
		 */
		__asm__(".set\tnoreorder\n\t"
			".set\tnoat\n"
			"1:\tand\t$1,%2,%1\n\t"
			"beq\t$0,$1,2f\n\t"
			"sll\t%2,%2,1\n\t"
			"bne\t$0,%2,1b\n\t"
			"addiu\t%0,%0,1\n\t"
			".set\tat\n\t"
			".set\treorder\n"
			: "=r" (set)
			: "r" (*p >> bit),
			  "r" (1),
			  "0" (0));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern inline unsigned long ffz(unsigned long word)
{
	unsigned int	__res;
	unsigned int	mask = 1;

	__asm__ __volatile__ (
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"li\t%2,1\n"
		"1:\tand\t$1,%2,%1\n\t"
		"beq\t$0,$1,2f\n\t"
		"sll\t%2,%2,1\n\t"
		"bne\t$0,%2,1b\n\t"
		"add\t%0,%0,1\n\t"
		".set\tat\n\t"
		".set\treorder\n"
		"2:\n\t"
		: "=r" (__res), "=r" (word), "=r" (mask)
		: "1" (~(word)),
		  "2" (mask),
		  "0" (0)
		: "$1");

	return __res;
}

#endif /* _ASM_MIPS_BITOPS_H */
