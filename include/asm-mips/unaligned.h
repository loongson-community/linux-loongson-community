/*
 * Inline functions to do unaligned accesses.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#ifndef __ASM_MIPS_UNALIGNED_H
#define __ASM_MIPS_UNALIGNED_H

#if 0
/*
 * The following macros are the portable and efficient versions of
 * get_unaligned()/put_unaligned().  They work but due to some lackings in the
 * MIPS machine description of GCC generate only good code only for accessing
 * word sized data.  We only use get_unaligned() for accessing unaligned ints
 * and then GCC makes full score anyway ...
 */
#define get_unaligned(ptr)                                              \
	({                                                              \
		struct __unal {                                         \
			__typeof__(*(ptr)) __x __attribute__((packed)); \
		};                                                      \
                                                                        \
		((struct __unal *)(ptr))->__x;                          \
	})

#define put_unaligned(ptr,val)                                          \
	({                                                              \
		struct __unal {                                         \
			__typeof__(*(ptr)) __x __attribute__((packed)); \
		};                                                      \
                                                                        \
		((struct __unal *)(ptr))->__x = (val);                  \
	})
#else

/* 
 * The main single-value unaligned transfer routines.
 */
#define get_unaligned(ptr) \
	((__typeof__(*(ptr)))__get_unaligned((ptr), sizeof(*(ptr))))
#define put_unaligned(x,ptr) \
	__put_unaligned((unsigned long)(x), (ptr), sizeof(*(ptr)))

/*
 * This is a silly but good way to make sure that
 * the get/put functions are indeed always optimized,
 * and that we use the correct sizes.
 */
extern void bad_unaligned_access_length(void);

/*
 * Load unaligned doubleword.
 */
extern __inline__ unsigned long __uld(const unsigned long long * __addr)
{
	unsigned long long __res;

	__asm__("uld\t%0,(%1)"
		:"=&r" (__res)
		:"r" (__addr));

	return __res;
}

/*
 * Load unaligned word.
 */
extern __inline__ unsigned long __ulw(const unsigned int * __addr)
{
	unsigned long __res;

	__asm__("ulw\t%0,(%1)"
		:"=&r" (__res)
		:"r" (__addr));

	return __res;
}

/*
 * Load unaligned halfword.
 */
extern __inline__ unsigned long __ulh(const unsigned short * __addr)
{
	unsigned long __res;

	__asm__("ulh\t%0,(%1)"
		:"=&r" (__res)
		:"r" (__addr));

	return __res;
}

/*
 * Store unaligned doubleword.
 */
extern __inline__ void __usd(unsigned long __val, unsigned long long * __addr)
{
	__asm__ __volatile__(
		"usd\t%0,(%1)"
		: /* No results */
		:"r" (__val),
		 "r" (__addr));
}

/*
 * Store unaligned word.
 */
extern __inline__ void __usw(unsigned long __val, unsigned int * __addr)
{
	__asm__ __volatile__(
		"usw\t%0,(%1)"
		: /* No results */
		:"r" (__val),
		 "r" (__addr));
}

/*
 * Store unaligned halfword.
 */
extern __inline__ void __ush(unsigned long __val, unsigned short * __addr)
{
	__asm__ __volatile__(
		"ush\t%0,(%1)"
		: /* No results */
		:"r" (__val),
		 "r" (__addr));
}

extern inline unsigned long __get_unaligned(const void *ptr, size_t size)
{
	unsigned long val;
	switch (size) {
	      case 1:
		val = *(const unsigned char *)ptr;
		break;
	      case 2:
		val = __ulh((const unsigned short *)ptr);
		break;
	      case 4:
		val = __ulw((const unsigned int *)ptr);
		break;
	      case 8:
		val = __uld((const unsigned long long *)ptr);
		break;
	      default:
		bad_unaligned_access_length();
	}
	return val;
}

extern inline void __put_unaligned(unsigned long val, void *ptr, size_t size)
{
	switch (size) {
	      case 1:
		*(unsigned char *)ptr = (val);
	        break;
	      case 2:
		__ush(val, (unsigned short *)ptr);
		break;
	      case 4:
		__usw(val, (unsigned int *)ptr);
		break;
	      case 8:
		__usd(val, (unsigned long long *)ptr);
		break;
	      default:
	    	bad_unaligned_access_length();
	}
}
#endif

#endif /* __ASM_MIPS_UNALIGNED_H */
