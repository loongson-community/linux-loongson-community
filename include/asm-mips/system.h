/*
 * include/asm-mips/system.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle, Paul M. Antoine
 */
#ifndef __ASM_MIPS_SYSTEM_H
#define __ASM_MIPS_SYSTEM_H

#include <linux/linkage.h>
#include <asm/sgidefs.h>

/*
 * sti/cli/save_flags use a memory clobber to make shure GCC doesn't
 * move memory references around calls to these functions.
 */
extern __inline__ void
sti(void)
{
    __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n\t"
	"mfc0\t$1,$12\n\t"
	"ori\t$1,1\n\t"
	"mtc0\t$1,$12\n\t"
	".set\tat\n\t"
	".set\treorder"
	: /* no outputs */
	: /* no inputs */
	: "$1","memory");
}

/*
 * For cli() we have to make shure that the new c0_status value has
 * really arrived in the status register at the end of the inline
 * function using worst case scheduling.  The worst case is the R4000
 * which needs three nops.
 */
extern __inline__ void
cli(void)
{
    __asm__ __volatile__(
	".set\tnoreorder\n\t"
	".set\tnoat\n\t"
	"mfc0\t$1,$12\n\t"
	"ori\t$1,1\n\t"
	"xori\t$1,1\n\t"
	"mtc0\t$1,$12\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	".set\tat\n\t"
	".set\treorder"
	: /* no outputs */
	: /* no inputs */
	: "$1","memory");
}

#define save_flags(x)                    \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"mfc0\t%0,$12\n\t"               \
	".set\treorder"                  \
	: "=r" (x)                       \
	: /* no inputs */                \
	: "memory")

extern void __inline__
restore_flags(int flags)
{
    __asm__ __volatile__(
	".set\tnoreorder\n\t"
	"mtc0\t%0,$12\n\t"
	"nop\n\t"
	"nop\n\t"
	"nop\n\t"
	".set\treorder"
	: /* no output */
	: "r" (flags)
	: "memory");
}

#if (_MIPS_ISA == _MIPS_ISA_MIPS2) || (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS5)
#define sync_mem()                       \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"sync\n\t"                       \
	".set\treorder"                  \
        : /* no output */                \
	: /* no input */                 \
	: "memory")
#else
/*
 * FIXME: Don't really know what to do here for the R[236]000's.
 *        Should probably bfc0 until write buffer is empty? - PMA
 *        Not shure if wb flushing is really required but sounds reasonable.
 *        The code below does this for R2000/R3000. - Ralf
 */
#define sync_mem()                       \
__asm__ __volatile__(                    \
	".set\tnoreorder\n\t"            \
	"nop;nop;nop;nop;\n"             \
	"1:\tbc0f\t1b\n\t"               \
	"nop\n\t"                        \
	".set\treorder"                  \
        : /* no output */                \
	: /* no input */                 \
	: "memory")
#endif

/*
 * switch_to(n) should switch tasks to task nr n, first
 * checking that n isn't the current task, in which case it does nothing.
 */
struct task_struct;
asmlinkage void resume(struct task_struct *tsk, int offset);

/*
 * FIXME: resume() assumes current == prev
 */
#define switch_to(prev,next)                                    \
	resume(next, ((int)(&((struct task_struct *)0)->tss)));

/*
 * The 8 and 16 bit variants have to disable interrupts temporarily.
 * Both are currently unused.
 */
extern __inline__ unsigned long xchg_u8(volatile char * m, unsigned long val)
{
	unsigned long flags, retval;

	save_flags(flags);
	cli();
	retval = *m;
	*m = val;
	restore_flags(flags);

	return retval;
}

extern __inline__ unsigned long xchg_u16(volatile short * m, unsigned long val)
{
	unsigned long flags, retval;

	save_flags(flags);
	cli();
	retval = *m;
	*m = val;
	restore_flags(flags);

	return retval;
}

/*
 * For 32 and 64 bit operands we can take advantage of ll and sc.
 * FIXME: This doesn't work for R3000 machines.
 */
extern __inline__ unsigned long xchg_u32(volatile int * m, unsigned long val)
{
#if (_MIPS_ISA == _MIPS_ISA_MIPS2) || (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS5)
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
#else /* FIXME: Brain-dead approach, but then again, I AM hacking - PMA */
	unsigned long flags, retval;

	save_flags(flags);
	cli();
	retval = *m;
	*m = val;
	restore_flags(flags);

#endif /* Processor-dependent optimization */
	return val;
}

/*
 * Only used for 64 bit kernel.
 */
extern __inline__ unsigned long xchg_u64(volatile long * m, unsigned long val)
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

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 *
 * This only works if the compiler isn't horribly bad at optimizing.
 * gcc-2.5.8 reportedly can't handle this, but I define that one to
 * be dead anyway.
 */
extern void __xchg_called_with_bad_pointer(void);

static __inline__ unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			return xchg_u8(ptr, x);
		case 2:
			return xchg_u16(ptr, x);
		case 4:
			return xchg_u32(ptr, x);
		case 8:
			return xchg_u64(ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;
}

extern unsigned long IRQ_vectors[16];
extern unsigned long exception_handlers[32];

#define set_int_vector(n,addr) \
	IRQ_vectors[n] = (unsigned long) (addr)

#define set_except_vector(n,addr) \
	exception_handlers[n] = (unsigned long) (addr)

/*
 * Reset the machine.
 */
extern void (*hard_reset_now)(void);

#endif /* __ASM_MIPS_SYSTEM_H */
