#ifndef _ASM_GENERIC_BITOPS_H
#define _ASM_GENERIC_BITOPS_H

/*
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assembly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * Also note, these routines assume that you have 32 bit integers.
 * You will have to change this if you are trying to port Linux to the
 * Alpha architecture or to a Cray.  :-)
 * 
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */

#ifdef __USE_GENERIC_set_bit
extern __inline__ int set_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*a = addr;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	cli();
	retval = (mask & *a) != 0;
	*a |= mask;
	sti();
	return retval;
}
#endif

#ifdef __USE_GENERIC_clear_bit
extern __inline__ int clear_bit(int nr, void * addr)
{
	int	mask, retval;
	int	*a = addr;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	cli();
	retval = (mask & *a) != 0;
	*a &= ~mask;
	sti();
	return retval;
}
#endif

#ifdef __USE_GENERIC_test_bit
extern __inline__ int test_bit(int nr, void * addr)
{
	int	mask;
	int	*a = addr;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *a) != 0);
}
#endif

#ifdef __USE_GENERIC_find_first_zero_bit
#error "Generic find_first_zero_bit() not written yet."
#endif

#ifdef __USE_GENERIC_find_next_zero_bit
#error "Generic find_next_zero_bit() not written yet."
#endif

#endif /* _ASM_GENERIC_BITOPS_H */
