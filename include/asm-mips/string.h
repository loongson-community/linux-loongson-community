/*
 * include/asm-mips/string.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1994, 1995, 1996 by Ralf Baechle
 */
#ifndef __ASM_MIPS_STRING_H
#define __ASM_MIPS_STRING_H

#ifdef __KERNEL__

#include <linux/linkage.h>
#include <asm/sgidefs.h>

#define __HAVE_ARCH_STRCPY
extern __inline__ char *strcpy(char *__dest, __const__ char *__src)
{
  char *__xdest = __dest;

  __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n"
	"1:\tlbu\t$1,(%1)\n\t"
	"addiu\t%1,1\n\t"
	"sb\t$1,(%0)\n\t"
	"bnez\t$1,1b\n\t"
	"addiu\t%0,1\n\t"
	".set\tat\n\t"
	".set\treorder"
	: "=r" (__dest), "=r" (__src)
        : "0" (__dest), "1" (__src)
	: "$1","memory");

  return __xdest;
}

#define __HAVE_ARCH_STRNCPY
extern __inline__ char *strncpy(char *__dest, __const__ char *__src, size_t __n)
{
  char *__xdest = __dest;

  if (__n == 0)
    return __xdest;

  __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n"
	"1:\tlbu\t$1,(%1)\n\t"
	"subu\t%2,1\n\t"
	"sb\t$1,(%0)\n\t"
	"beqz\t$1,2f\n\t"
	"addiu\t%0,1\n\t"
	"bnez\t%2,1b\n\t"
	"addiu\t%1,1\n"
	"2:\n\t"
	".set\tat\n\t"
	".set\treorder"
        : "=r" (__dest), "=r" (__src), "=r" (__n)
        : "0" (__dest), "1" (__src), "2" (__n)
        : "$1","memory");

  return __dest;
}

#define __HAVE_ARCH_STRCMP
extern __inline__ int strcmp(__const__ char *__cs, __const__ char *__ct)
{
  int __res;

  __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n\t"
	"lbu\t%2,(%0)\n"
	"1:\tlbu\t$1,(%1)\n\t"
	"addiu\t%0,1\n\t"
	"bne\t$1,%2,2f\n\t"
	"addiu\t%1,1\n\t"
	"bnez\t%2,1b\n\t"
	"lbu\t%2,(%0)\n\t"
#if _MIPS_ISA == _MIPS_ISA_MIPS1
	"nop\n\t"
#endif
	"move\t%2,$1\n"
	"2:\tsubu\t%2,$1\n"
	"3:\t.set\tat\n\t"
	".set\treorder"
	: "=r" (__cs), "=r" (__ct), "=r" (__res)
	: "0" (__cs), "1" (__ct)
	: "$1");

  return __res;
}

#define __HAVE_ARCH_STRNCMP
extern __inline__ int strncmp(__const__ char *__cs, __const__ char *__ct, size_t __count)
{
  char __res;

  __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n"
       	"1:\tlbu\t%3,(%0)\n\t"
	"beqz\t%2,2f\n\t"
        "lbu\t$1,(%1)\n\t"
       	"subu\t%2,1\n\t"
        "bne\t$1,%3,3f\n\t"
        "addiu\t%0,1\n\t"
        "bnez\t%3,1b\n\t"
        "addiu\t%1,1\n"
	"2:\tmove\t%3,$1\n"
	"3:\tsubu\t%3,$1\n\t"
	".set\tat\n\t"
	".set\treorder"
        : "=r" (__cs), "=r" (__ct), "=r" (__count), "=r" (__res)
        : "0" (__cs), "1" (__ct), "2" (__count)
	: "$1");

  return __res;
}

#define __HAVE_ARCH_MEMSET
/*
 * Ok, this definately looks braindead.  I tried several other variants
 * some of which GCC wasn't able to optimize or which made GCC consume
 * extreme amounts of memory.
 * This code attempts never to generate address errors which require
 * expensive software emulation.  For this purpose GCC's __alignof__
 * seems to be perfect.  Unfortunately GCC 2.7.2 complains about
 * __alignof__(*p) when p is a pointer to void.  For now I ignore these
 * warnings.
 */

extern void __generic_memset_b(void *__s, int __c, size_t __count);
extern void __generic_memset_dw(void *__s, unsigned long long __c,
                                size_t __count);

/*
 * The constant c handling looks wired but it combines minimal code
 * size with fast execution.
 */
#define __generic_memset(s, c, count, const_c) \
({if(const_c) { \
	unsigned long long __dwc; \
\
	__dwc = c & 0xff; \
	__dwc = (__dwc << 8) | __dwc; \
	__dwc = (__dwc << 16) | __dwc; \
	__dwc = (__dwc << 32) | __dwc; \
	__generic_memset_dw(s, __dwc, count); \
	} \
else \
	__generic_memset_b(s, c, count); \
})

extern __inline__ void __const_count_memset1(void *__s, int __c, size_t __count,
                                             int __const_c)
{
	switch(__count) {
	case 0:	return;
	case 1:	*(0+(char *)__s) = __c;
		return;
	case 2:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		return;
	case 3:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		return;
	case 4:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		*(3+(char *)__s) = __c;
		return;
	case 5:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		*(3+(char *)__s) = __c;
		*(4+(char *)__s) = __c;
		return;
	case 6:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		*(3+(char *)__s) = __c;
		*(4+(char *)__s) = __c;
		*(5+(char *)__s) = __c;
		return;
	case 7:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		*(3+(char *)__s) = __c;
		*(4+(char *)__s) = __c;
		*(5+(char *)__s) = __c;
		*(6+(char *)__s) = __c;
		return;
	case 8:	*(0+(char *)__s) = __c;
		*(1+(char *)__s) = __c;
		*(2+(char *)__s) = __c;
		*(3+(char *)__s) = __c;
		*(4+(char *)__s) = __c;
		*(5+(char *)__s) = __c;
		*(6+(char *)__s) = __c;
		*(7+(char *)__s) = __c;
		return;
	}
	__generic_memset(__s, __c, __count, __const_c);
	return;
}

extern __inline__ void __const_count_memset2(void *__s, int __c, size_t __count,
                                             int __const_c)
{
	switch(__count) {
	case 0:	return;
	case 2:	*(0+(short *)__s) = 0x0101 * __c;
		return;
	case 4:	*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		return;
	case 6:	*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		return;
	case 8:	*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		*(3+(short *)__s) = 0x0101 * __c;
		return;
	case 10:*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		*(3+(short *)__s) = 0x0101 * __c;
		*(4+(short *)__s) = 0x0101 * __c;
		return;
	case 12:*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		*(3+(short *)__s) = 0x0101 * __c;
		*(4+(short *)__s) = 0x0101 * __c;
		*(5+(short *)__s) = 0x0101 * __c;
		return;
	case 14:*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		*(3+(short *)__s) = 0x0101 * __c;
		*(4+(short *)__s) = 0x0101 * __c;
		*(5+(short *)__s) = 0x0101 * __c;
		*(6+(short *)__s) = 0x0101 * __c;
		return;
	case 16:*(0+(short *)__s) = 0x0101 * __c;
		*(1+(short *)__s) = 0x0101 * __c;
		*(2+(short *)__s) = 0x0101 * __c;
		*(3+(short *)__s) = 0x0101 * __c;
		*(4+(short *)__s) = 0x0101 * __c;
		*(5+(short *)__s) = 0x0101 * __c;
		*(6+(short *)__s) = 0x0101 * __c;
		*(7+(short *)__s) = 0x0101 * __c;
		return;
	}
	__generic_memset(__s, __c, __count, __const_c);
	return;
}

extern __inline__ void __const_count_memset4(void *__s, int __c, size_t __count,
                                             int __const_c)
{
	switch(__count) {
	case 0:	return;
	case 4:	*(0+(int *)__s) = 0x01010101 * __c;
		return;
	case 8:	*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		return;
	case 12:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		return;
	case 16:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		*(3+(int *)__s) = 0x01010101 * __c;
		return;
	case 20:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		*(3+(int *)__s) = 0x01010101 * __c;
		*(4+(int *)__s) = 0x01010101 * __c;
		return;
	case 24:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		*(3+(int *)__s) = 0x01010101 * __c;
		*(4+(int *)__s) = 0x01010101 * __c;
		*(5+(int *)__s) = 0x01010101 * __c;
		return;
	case 28:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		*(3+(int *)__s) = 0x01010101 * __c;
		*(4+(int *)__s) = 0x01010101 * __c;
		*(5+(int *)__s) = 0x01010101 * __c;
		*(6+(int *)__s) = 0x01010101 * __c;
		return;
	case 32:*(0+(int *)__s) = 0x01010101 * __c;
		*(1+(int *)__s) = 0x01010101 * __c;
		*(2+(int *)__s) = 0x01010101 * __c;
		*(3+(int *)__s) = 0x01010101 * __c;
		*(4+(int *)__s) = 0x01010101 * __c;
		*(5+(int *)__s) = 0x01010101 * __c;
		*(6+(int *)__s) = 0x01010101 * __c;
		*(7+(int *)__s) = 0x01010101 * __c;
		return;
	}
	__generic_memset(__s, __c, __count, __const_c);
	return;
}

extern __inline__ void __const_count_memset8(void *__s, int __c, size_t __count,
                                             int __const_c)
{
	unsigned long long __dwc;

	__dwc = __c & 0xff;
	__dwc = (__dwc << 8) | __dwc;
	__dwc = (__dwc << 16) | __dwc;
	__dwc = (__dwc << 32) | __dwc;
	switch(__count) {
	case 0:	return;
	case 8:	*(0+(long long *)__s) = __dwc;
		return;
	case 16:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		return;
	case 24:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		return;
	case 32:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		*(3+(long long *)__s) = __dwc;
		return;
	case 40:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		*(3+(long long *)__s) = __dwc;
		*(4+(long long *)__s) = __dwc;
		return;
	case 48:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		*(3+(long long *)__s) = __dwc;
		*(4+(long long *)__s) = __dwc;
		*(5+(long long *)__s) = __dwc;
		return;
	case 56:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		*(3+(long long *)__s) = __dwc;
		*(4+(long long *)__s) = __dwc;
		*(5+(long long *)__s) = __dwc;
		*(6+(long long *)__s) = __dwc;
		return;
	case 64:*(0+(long long *)__s) = __dwc;
		*(1+(long long *)__s) = __dwc;
		*(2+(long long *)__s) = __dwc;
		*(3+(long long *)__s) = __dwc;
		*(4+(long long *)__s) = __dwc;
		*(5+(long long *)__s) = __dwc;
		*(6+(long long *)__s) = __dwc;
		*(7+(long long *)__s) = __dwc;
		return;
	}
	__generic_memset(__s, __c, __count, __const_c);
	return;
}

extern __inline__ void * __const_count_memset(void *__s, int __c,
                                              size_t __count, int __align,
                                              int __const_c)
{
	switch(__align) {
	/*
	 * We only want this for the 64 bit CPUs; this gets
	 * too bloated on 32 bit.
	 */
	case 1: __const_count_memset1(__s, __c, __count, __const_c);
		return __s;
	case 2: __const_count_memset2(__s, __c, __count, __const_c);
		return __s;
	case 4: __const_count_memset4(__s, __c, __count, __const_c);
		return __s;
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
	case 8: __const_count_memset8(__s, __c, __count, __const_c);
		return __s;
#endif
	}
	__generic_memset(__s, __c, __count, __const_c);
	return __s;
}

#define memset(s, c, count) \
(__builtin_constant_p(count) ? \
 __const_count_memset((s),(c),(count),__alignof__(*(s)), \
                      __builtin_constant_p(c)) : \
 __generic_memset((s),(c),(count),__builtin_constant_p(c)))

#define __HAVE_ARCH_MEMCPY
extern void __memcpy(void *__to, __const__ void *__from, size_t __n);
extern __inline__ void *memcpy(void *__to, __const__ void *__from, size_t __n)
{
	__memcpy(__to, __from, __n);

	return __to;
}

#define __HAVE_ARCH_MEMMOVE
extern void __memmove(void *__dest, __const__ void *__src, size_t __n);
extern __inline__ void *memmove(void *__dest, __const__ void *__src, size_t __n)
{
	__memmove(__dest, __src, __n);

	return __dest;
}

#define __HAVE_ARCH_BCOPY
extern __inline__ char *bcopy(__const__ char *__src, char *__dest, size_t __count)
{
	__memmove(__dest, __src, __count);

	return __dest;
}

#define __HAVE_ARCH_MEMSCAN
extern __inline__ void *memscan(void *__addr, int __c, size_t __size)
{
	char *__end = (char *)__addr + __size;

	if (!__size)
		return __addr;
	__asm__(".set\tnoat\n"
		"1:\tlbu\t$1,(%0)\n\t"
		".set\tnoreorder\n\t"
		"beq\t$1,%3,2f\n\t"
		"addiu\t%0,1\t\t\t# delay slot\n\t"
		".set\treorder\n\t"
		".set\tat\n\t"
		"bne\t%0,%2,1b\n\t"
		"2:\n"
		: "=r" (__addr)
		: "0" (__addr), "1" (__end), "r" (__c)
		: "$1");

	return __addr;
}

#endif /* __KERNEL__ */

#endif /* __ASM_MIPS_STRING_H */
