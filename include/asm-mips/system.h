/*
 * include/asm-mips/system.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 */
#ifndef __ASM_MIPS_SYSTEM_H
#define __ASM_MIPS_SYSTEM_H

#if defined (__R4000__)
#define sti()                            \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,$12\n\t"               \
	"ori\t$1,0x1f\n\t"               \
	"xori\t$1,0x1e\n\t"              \
	"mtc0\t$1,$12\n\t"               \
	".set\tat\n\t"                   \
	".set\treorder"                  \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")

#define cli()                            \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,$12\n\t"               \
	"ori\t$1,1\n\t"                  \
	"xori\t$1,1\n\t"                 \
	"mtc0\t$1,$12\n\t"               \
	"nop\n\t"                        \
	"nop\n\t"                        \
	"nop\n\t"                        \
	".set\tat\n\t"                   \
	".set\treorder"                  \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")

#else /* !defined (__R4000__) */
/*
 * Untested goodies for the R3000 based DECstation et al.
 */
#define sti()                            \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,$12\n\t"               \
	"ori\t$1,0x01\n\t"               \
	"mtc0\t$1,$12\n\t"               \
	".set\tat\n\t"                   \
	".set\treorder"                  \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")

#define cli()                            \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,$12\n\t"               \
	"ori\t$1,1\n\t"                  \
	"xori\t$1,1\n\t"                 \
	"mtc0\t$1,$12\n\t"               \
	".set\tat\n\t"                   \
	".set\treorder"                  \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")
#endif /* !defined (__R4000__) */

#define nop() __asm__ __volatile__ ("nop")

#define save_flags(x)                    \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"mfc0\t%0,$12\n\t"               \
	".set\treorder"                  \
	: "=r" (x))                      \

#define restore_flags(x)                 \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"mtc0\t%0,$12\n\t"               \
	".set\treorder"                  \
	: /* no output */                \
	: "r" (x))                       \

#define sync_mem()                       \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"sync\n\t"                       \
	".set\treorder")                 \

/*
 * The 8 and 16 bit variants have to disable interrupts temporarily.
 * Both are currently unused.
 */
extern inline unsigned long xchg_u8(char * m, unsigned long val)
{
	unsigned long flags, retval;

	save_flags(flags);
	sti();
	retval = *m;
	*m = val;
	restore_flags(flags);

	return retval;
}

extern inline unsigned long xchg_u16(short * m, unsigned long val)
{
	unsigned long flags, retval;

	save_flags(flags);
	sti();
	retval = *m;
	*m = val;
	restore_flags(flags);

	return retval;
}

/*
 * For 32 and 64 bit operands we can take advantage of ll and sc.
 * FIXME: This doesn't work for R3000 machines.
 */
extern inline unsigned long xchg_u32(int * m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"ll\t%0,(%1)\n"
		"1:\tmove\t$1,%2\n\t"
		"sc\t$1,(%1)\n\t"
		"beqzl\t$1,1b\n\t"
		"ll\t%0,(%1)\n\t"
		".set\tat\n\t"
		".set\treorder"
		: "=r" (val), "=r" (m), "=r" (dummy)
		: "1" (m), "2" (val));

	return val;
}

/*
 * Only used for 64 bit kernel.
 */
extern inline unsigned long xchg_u64(long * m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"lld\t%0,(%1)\n"
		"1:\tmove\t$1,%2\n\t"
		"scd\t$1,(%1)\n\t"
		"beqzl\t$1,1b\n\t"
		"ll\t%0,(%1)\n\t"
		".set\tat\n\t"
		".set\treorder"
		: "=r" (val), "=r" (m), "=r" (dummy)
		: "1" (m), "2" (val));

	return val;
}

extern inline void * xchg_ptr(void * m, void * val)
{
#if __mips == 3
	return (void *) xchg_u64(m, (unsigned long) val);
#else
	return (void *) xchg_u32(m, (unsigned long) val);
#endif
}

extern unsigned long IRQ_vectors[16];
extern unsigned long exception_handlers[32];

#define set_int_vector(n,addr) \
	IRQ_vectors[n] = (unsigned long) (addr)

#define set_except_vector(n,addr) \
	exception_handlers[n] = (unsigned long) (addr)

#endif /* __ASM_MIPS_SYSTEM_H */
