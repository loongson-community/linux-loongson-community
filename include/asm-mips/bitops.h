/*
 * include/asm-mips/bitops.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1994, 1995, 1996  Ralf Baechle
 */
#ifndef __ASM_MIPS_BITOPS_H
#define __ASM_MIPS_BITOPS_H

#include <asm/sgidefs.h>

/*
 * Note that the bit operations are defined on arrays of 32 bit sized
 * elements.  With respect to a future 64 bit implementation it is
 * wrong to use long *.  Use u32 * or int *.
 */
extern __inline__ int set_bit(int nr, void *addr);
extern __inline__ int clear_bit(int nr, void *addr);
extern __inline__ int change_bit(int nr, void *addr);
extern __inline__ int test_bit(int nr, const void *addr);
extern __inline__ int find_first_zero_bit (void *addr, unsigned size);
extern __inline__ int find_next_zero_bit (void * addr, int size, int offset);
extern __inline__ unsigned long ffz(unsigned long word);

#if (_MIPS_ISA == _MIPS_ISA_MIPS2) || (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS5)

/*
 * These functions for MIPS ISA > 1 are interrupt and SMP proof and
 * interrupt friendly
 */
#include <asm/mipsregs.h>

/*
 * The following functions will only work for the R4000!
 */
extern __inline__ int set_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += ((nr >> 3) & ~3);
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw|mask));

	return retval;
}

extern __inline__ int clear_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += ((nr >> 3) & ~3);
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw & ~mask));

	return retval;
}

extern __inline__ int change_bit(int nr, void *addr)
{
	int	mask, retval, mw;

	addr += ((nr >> 3) & ~3);
	mask = 1 << (nr & 0x1f);
	do {
		mw = load_linked(addr);
		retval = (mask & mw) != 0;
		}
	while (!store_conditional(addr, mw ^ mask));

	return retval;
}

#else /* MIPS I */

#ifdef __KERNEL__
/*
 * These functions are only used for MIPS ISA 1 CPUs.  Since I don't
 * believe that someone ever will run Linux/SMP on such a beast I don't
 * worry about making them SMP proof.
 */
#include <asm/system.h>

/*
 * Only disable interrupt for kernel mode stuff to keep usermode stuff
 * that dares to use kernel include files alive.
 */
#define __flags unsigned long flags
#define __cli() cli()
#define __save_flags(x) save_flags(x)
#define __restore_flags(x) restore_flags(x)
#else
#define __flags
#define __cli()
#define __save_flags(x)
#define __restore_flags(x)
#endif /* __KERNEL__ */

extern __inline__ int set_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*a = addr;
	__flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	__save_flags(flags);
	__cli();
	retval = (mask & *a) != 0;
	*a |= mask;
	__restore_flags(flags);

	return retval;
}

extern __inline__ int clear_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*a = addr;
	__flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	__save_flags(flags);
	__cli();
	retval = (mask & *a) != 0;
	*a &= ~mask;
	__restore_flags(flags);

	return retval;
}

extern __inline__ int change_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*a = addr;
	__flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	__save_flags(flags);
	__cli();
	retval = (mask & *a) != 0;
	*a ^= mask;
	__restore_flags(flags);

	return retval;
}

#undef __flags
#undef __cli()
#undef __save_flags(x)
#undef __restore_flags(x)

#endif /* MIPS I */

extern __inline__ int test_bit(int nr, const void *addr)
{
	return ((1UL << (nr & 31)) & (((const unsigned int *) addr)[nr >> 5])) != 0;
}

extern __inline__ int find_first_zero_bit (void *addr, unsigned size)
{
	unsigned long dummy;
	int res;

	if (!size)
		return 0;

	__asm__ (".set\tnoreorder\n\t"
		".set\tnoat\n"
		"1:\tsubu\t$1,%6,%0\n\t"
		"blez\t$1,2f\n\t"
		"lw\t$1,(%5)\n\t"
		"addiu\t%5,4\n\t"
#if (_MIPS_ISA == _MIPS_ISA_MIPS2) || (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS5)
		"beql\t%1,$1,1b\n\t"
		"addiu\t%0,32\n\t"
#else
		"addiu\t%0,32\n\t"
		"beq\t%1,$1,1b\n\t"
		"nop\n\t"
		"subu\t%0,32\n\t"
#endif
#ifdef __MIPSEB__
#error "Fix this for big endian"
#endif /* __MIPSEB__ */
		"li\t%1,1\n"
		"1:\tand\t%2,$1,%1\n\t"
		"beqz\t%2,2f\n\t"
		"sll\t%1,%1,1\n\t"
		"bnez\t%1,1b\n\t"
		"add\t%0,%0,1\n\t"
		".set\tat\n\t"
		".set\treorder\n"
		"2:"
		: "=r" (res),
		  "=r" (dummy),
		  "=r" (addr)
		: "0" ((signed int) 0),
		  "1" ((unsigned int) 0xffffffff),
		  "2" (addr),
		  "r" (size)
		: "$1");

	return res;
}

extern __inline__ int find_next_zero_bit (void * addr, int size, int offset)
{
	unsigned int *p = ((unsigned int *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;
	unsigned long dummy;
	
	if (bit) {
		/*
		 * Look for zero in first byte
		 */
#ifdef __MIPSEB__
#error "Fix this for big endian byte order"
#endif
		__asm__(".set\tnoreorder\n\t"
			".set\tnoat\n"
			"1:\tand\t$1,%4,%1\n\t"
			"beqz\t$1,1f\n\t"
			"sll\t%1,%1,1\n\t"
			"bnez\t%1,1b\n\t"
			"addiu\t%0,1\n\t"
			".set\tat\n\t"
			".set\treorder\n"
			"1:"
			: "=r" (set),
			  "=r" (dummy)
			: "0" (0),
			  "1" (1 << bit),
			  "r" (*p)
			: "$1");
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = find_first_zero_bit(p, size - 32 * (p - (unsigned int *) addr));
	return offset + set + res;
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
	unsigned int	__res;
	unsigned int	mask = 1;

	__asm__ (
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"move\t%0,$0\n"
		"1:\tand\t$1,%2,%1\n\t"
		"beqz\t$1,2f\n\t"
		"sll\t%1,1\n\t"
		"bnez\t%1,1b\n\t"
		"addiu\t%0,1\n\t"
		".set\tat\n\t"
		".set\treorder\n"
		"2:\n\t"
		: "=&r" (__res), "=r" (mask)
		: "r" (word), "1" (mask)
		: "$1");

	return __res;
}

#ifdef __KERNEL__
#ifdef __MIPSEB__
#error "Aieee...  fix these sources for big endian machines"
#endif /* __MIPSEB__ */

#define ext2_set_bit                 set_bit
#define ext2_clear_bit               clear_bit
#define ext2_test_bit                test_bit
#define ext2_find_first_zero_bit     find_first_zero_bit
#define ext2_find_next_zero_bit      find_next_zero_bit

#endif /* __KERNEL__ */

#endif /* __ASM_MIPS_BITOPS_H */
