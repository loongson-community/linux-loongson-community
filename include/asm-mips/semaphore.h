/*
 * SMP- and interrupt-safe semaphores..
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright 1996  Linus Torvalds, Ralf Baechle
 */
#ifndef __ASM_MIPS_SEMAPHORE_H
#define __ASM_MIPS_SEMAPHORE_H

#include <asm/atomic.h>

struct semaphore {
	atomic_t count;
	atomic_t waiting;
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, NULL })

extern void __down(struct semaphore * sem);
extern void __up(struct semaphore * sem);

extern inline void down(struct semaphore * sem)
{
	while (1) {
		if (atomic_dec_return(&sem->count) >= 0)
			break;
		__down(sem);
	}
}

/*
 * Note! This is subtle. We jump to wake people up only if
 * the semaphore was negative (== somebody was waiting on it).
 */
extern inline void up(struct semaphore * sem)
{
	if (atomic_inc_return(&sem->count) <= 0)
		__up(sem);
}

#endif /* __ASM_MIPS_SEMAPHORE_H */
