/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 *
 * But use these as seldom as possible since they are much more slower
 * than regular operations.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 2000 by Ralf Baechle
 */
#ifndef __ASM_ATOMIC_H
#define __ASM_ATOMIC_H

#include <linux/config.h>

typedef struct { volatile int counter; } atomic_t;

#ifdef __KERNEL__
#define ATOMIC_INIT(i)    { (i) }

#define atomic_read(v)	((v)->counter)
#define atomic_set(v,i)	((v)->counter = (i))

#if !defined(CONFIG_CPU_HAS_LLSC)

#include <asm/system.h>

/*
 * The MIPS I implementation is only atomic with respect to
 * interrupts.  R3000 based multiprocessor machines are rare anyway ...
 */
extern __inline__ void atomic_add(int i, atomic_t * v)
{
	int	flags;

	save_flags(flags);
	cli();
	v->counter += i;
	restore_flags(flags);
}

extern __inline__ void atomic_sub(int i, atomic_t * v)
{
	int	flags;

	save_flags(flags);
	cli();
	v->counter -= i;
	restore_flags(flags);
}

extern __inline__ int atomic_add_return(int i, atomic_t * v)
{
	int	temp, flags;

	save_flags(flags);
	cli();
	temp = v->counter;
	temp += i;
	v->counter = temp;
	restore_flags(flags);

	return temp;
}

extern __inline__ int atomic_sub_return(int i, atomic_t * v)
{
	int	temp, flags;

	save_flags(flags);
	cli();
	temp = v->counter;
	temp -= i;
	v->counter = temp;
	restore_flags(flags);

	return temp;
}

#else

/*
 * ... while for MIPS II and better we can use ll/sc instruction.  This
 * implementation is SMP safe ...
 */

extern __inline__ void atomic_add(int i, atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1      # atomic_add\n"
		"     addu    %0, %2                  \n"
		"     sc      %0, %1                  \n"
		"     beqz    %0, 1b                  \n"
		: "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter));
}

extern __inline__ void atomic_sub(int i, atomic_t * v)
{
	unsigned long temp;

	__asm__ __volatile__(
		"1:   ll      %0, %1      # atomic_sub\n"
		"     subu    %0, %2                  \n"
		"     sc      %0, %1                  \n"
		"     beqz    %0, 1b                  \n"
		: "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter));
}

/*
 * Same as above, but return the result value
 */
extern __inline__ int atomic_add_return(int i, atomic_t * v)
{
	unsigned long temp, result;

	__asm__ __volatile__(
		".set push               # atomic_add_return\n"
		".set noreorder                             \n"
		"1:   ll      %1, %2                        \n"
		"     addu    %0, %1, %3                    \n"
		"     sc      %0, %2                        \n"
		"     beqz    %0, 1b                        \n"
		"     addu    %0, %1, %3                    \n"
		".set pop                                   \n"
		: "=&r" (result), "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter)
		: "memory");

	return result;
}

extern __inline__ int atomic_sub_return(int i, atomic_t * v)
{
	unsigned long temp, result;

	__asm__ __volatile__(
		".set push                                   \n"
		".set noreorder           # atomic_sub_return\n"
		"1:   ll    %1, %2                           \n"
		"     subu  %0, %1, %3                       \n"
		"     sc    %0, %2                           \n"
		"     beqz  %0, 1b                           \n"
		"     subu  %0, %1, %3                       \n"
		".set pop                                    \r"
		: "=&r" (result), "=&r" (temp), "=m" (v->counter)
		: "Ir" (i), "m" (v->counter)
		: "memory");

	return result;
}
#endif

#define atomic_dec_return(v) atomic_sub_return(1,(v))
#define atomic_inc_return(v) atomic_add_return(1,(v))

#define atomic_sub_and_test(i,v) (atomic_sub_return((i), (v)) == 0)
#define atomic_dec_and_test(v) (atomic_sub_return(1, (v)) == 0)

#define atomic_inc(v) atomic_add(1,(v))
#define atomic_dec(v) atomic_sub(1,(v))
#endif /* defined(__KERNEL__) */

#endif /* __ASM_ATOMIC_H */
