/*
 * SMP- and interrupt-safe semaphores helper functions.
 *
 * Copyright (C) 1996 Linus Torvalds
 * Copyright (C) 1999 Andrea Arcangeli
 * Copyright (C) 1999, 2001, 2002 Ralf Baechle
 * Copyright (C) 1999, 2001 Silicon Graphics, Inc.
 * Copyright (C) 2000 MIPS Technologies, Inc.
 */
#ifndef _ASM_SEMAPHORE_HELPER_H
#define _ASM_SEMAPHORE_HELPER_H

#include <linux/config.h>
#include <linux/errno.h>

#define sem_read(a) ((a)->counter)
#define sem_inc(a) (((a)->counter)++)
#define sem_dec(a) (((a)->counter)--)
/*
 * These two _must_ execute atomically wrt each other.
 */
static inline void wake_one_more(struct semaphore * sem)
{
	atomic_inc(&sem->waking);
}

#ifdef CONFIG_CPU_HAS_LLSC

static inline int waking_non_zero(struct semaphore *sem)
{
	int ret, tmp;

	__asm__ __volatile__(
	"1:	ll	%1, %2			# waking_non_zero	\n"
	"	blez	%1, 2f						\n"
	"	subu	%0, %1, 1					\n"
	"	sc	%0, %2						\n"
	"	beqz	%0, 1b						\n"
	"2:								\n"
	: "=r" (ret), "=r" (tmp), "+m" (sem->waking)
	: "0" (0));

	return ret;
}

#else /* !CONFIG_CPU_HAS_LLSC */

/*
 * It doesn't make sense, IMHO, to endlessly turn interrupts off and on again.
 * Do it once and that's it. ll/sc *has* it's advantages. HK
 */

static inline int waking_non_zero(struct semaphore *sem)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&semaphore_lock, flags);
	if (sem_read(&sem->waking) > 0) {
		sem_dec(&sem->waking);
		ret = 1;
	}
	spin_lock_irqrestore(&semaphore_lock, flags);

	return ret;
}
#endif /* !CONFIG_CPU_HAS_LLSC */

#ifdef CONFIG_CPU_HAS_LLDSCD

/*
 * waking_non_zero_interruptible:
 *	1	got the lock
 *	0	go to sleep
 *	-EINTR	interrupted
 *
 * We must undo the sem->count down_interruptible decrement
 * simultaneously and atomically with the sem->waking adjustment,
 * otherwise we can race with wake_one_more.
 *
 * This is accomplished by doing a 64-bit lld/scd on the 2 32-bit words.
 *
 * This is crazy.  Normally it's strictly forbidden to use 64-bit operations
 * in the 32-bit MIPS kernel.  In this case it's however ok because if an
 * interrupt has destroyed the upper half of registers sc will fail.
 * Note also that this will not work for MIPS32 CPUs!
 *
 * Pseudocode:
 *
 * If(sem->waking > 0) {
 *	Decrement(sem->waking)
 *	Return(SUCCESS)
 * } else If(signal_pending(tsk)) {
 *	Increment(sem->count)
 *	Return(-EINTR)
 * } else {
 *	Return(SLEEP)
 * }
 */

static inline int
waking_non_zero_interruptible(struct semaphore *sem, struct task_struct *tsk)
{
	long ret, tmp;

	__asm__ __volatile__(
	"	.set	push		# waking_non_zero_interruptible	\n"
	"	.set	mips3						\n"
	"	.set	noat						\n"
	"0:	lld	%1, %2						\n"
	"	li	%0, 0						\n"
	"	sll	$1, %1, 0					\n"
	"	blez	$1, 1f						\n"
	"	daddiu	%1, %1, -1					\n"
	"	li	%0, 1						\n"
	"	b	2f						\n"
	"1:	beqz	%3, 2f						\n"
	"	li	%0, %4						\n"
	"	dli	$1, 0x0000000100000000				\n"
	"	daddu	%1, %1, $1					\n"
	"2:	scd	%1, %2						\n"
	"	beqz	%1, 0b						\n"
	"	.set	pop"
	: "=&r" (ret), "=&r" (tmp), "=m" (*sem)
	: "r" (signal_pending(tsk)), "i" (-EINTR));

	return ret;
}

/*
 * waking_non_zero_trylock is unused.  we do everything in
 * down_trylock and let non-ll/sc hosts bounce around.
 */

static inline int waking_non_zero_trylock(struct semaphore *sem)
{
#if WAITQUEUE_DEBUG
	CHECK_MAGIC(sem->__magic);
#endif

	return 0;
}

#else /* !CONFIG_CPU_HAS_LLDSCD */

static inline int waking_non_zero_interruptible(struct semaphore *sem,
						struct task_struct *tsk)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&semaphore_lock, flags);
	if (sem_read(&sem->waking) > 0) {
		sem_dec(&sem->waking);
		ret = 1;
	} else if (signal_pending(tsk)) {
		sem_inc(&sem->count);
		ret = -EINTR;
	}
	spin_lock_irqrestore(&semaphore_lock, flags);

	return ret;
}

static inline int waking_non_zero_trylock(struct semaphore *sem)
{
	unsigned long flags;
	int ret = 1;

	spin_lock_irqsave(&semaphore_lock, flags);
	if (sem_read(&sem->waking) <= 0)
		sem_inc(&sem->count);
	else {
		sem_dec(&sem->waking);
		ret = 0;
	}
	spin_lock_irqrestore(&semaphore_lock, flags);

	return ret;
}

#endif /* !CONFIG_CPU_HAS_LLDSCD */

#endif /* _ASM_SEMAPHORE_HELPER_H */
