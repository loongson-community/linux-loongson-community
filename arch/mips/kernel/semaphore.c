/*
 * Generic semaphore code. Buyer beware. Do your own
 * specific changes in <asm/semaphore-helper.h>
 */
#include <linux/module.h>
#include <linux/sched.h>
#include <asm/semaphore-helper.h>

/*
 * Semaphores are implemented using a two-way counter: The "count" variable is
 * decremented for each process that tries to sleep, while the "waking" variable
 * is incremented when the "up()" code goes to wake up waiting processes.
 *
 * Notably, the inline "up()" and "down()" functions can efficiently test if
 * they need to do any extra work (up needs to do something only if count was
 * negative before the increment operation.
 *
 * waking_non_zero() (from asm/semaphore-helper.h) must execute atomically.
 *
 * When __up() is called, the count was negative before incrementing it, and we
 * need to wake up somebody.
 *
 * This routine adds one to the count of processes that need to wake up and
 * exit.  ALL waiting processes actually wake up but only the one that gets to
 * the "waking" field first will gate through and acquire the semaphore.  The
 * others will go back to sleep.
 *
 * Note that these functions are only called when there is contention on the
 * lock, and as such all this is the "non-critical" part of the whole semaphore
 * business. The critical part is the inline stuff in <asm/semaphore.h> where
 * we want to avoid any extra jumps and calls.
 */
void __up(struct semaphore *sem)
{
	wake_one_more(sem);
	wake_up(&sem->wait);
}

EXPORT_SYMBOL(__up);

/*
 * Perform the "down" function.  Return zero for semaphore acquired, return
 * negative for signalled out of the function.
 *
 * If called from down, the return is ignored and the wait loop is not
 * interruptible.  This means that a task waiting on a semaphore using "down()"
 * cannot be killed until someone does an "up()" on the semaphore.
 *
 * If called from down_interruptible, the return value gets checked upon return.
 * If the return value is negative then the task continues with the negative
 * value in the return register (it can be tested by the caller).
 *
 * Either form may be used in conjunction with "up()".
 */

void __down(struct semaphore * sem)
{
	struct task_struct *tsk = current;
	wait_queue_t wait;

	init_waitqueue_entry(&wait, tsk);
	tsk->state = TASK_UNINTERRUPTIBLE;
	add_wait_queue(&sem->wait, &wait);

	/*
	 * Ok, we're set up.  sem->count is known to be less than zero
	 * so we must wait.
	 *
	 * We can let go the lock for purposes of waiting.
	 * We re-acquire it after awaking so as to protect
	 * all semaphore operations.
	 *
	 * If "up()" is called before we call waking_non_zero() then
	 * we will catch it right away.  If it is called later then
	 * we will have to go through a wakeup cycle to catch it.
	 *
	 * Multiple waiters contend for the semaphore lock to see
	 * who gets to gate through and who has to wait some more.
	 */
	for (;;) {
		if (waking_non_zero(sem))
			break;
		schedule();
		tsk->state = TASK_UNINTERRUPTIBLE;
	}
	tsk->state = TASK_RUNNING;
	remove_wait_queue(&sem->wait, &wait);
}

EXPORT_SYMBOL(__down);

int __down_interruptible(struct semaphore * sem)
{
	struct task_struct *tsk = current;
	wait_queue_t wait;
	int ret = 0;

	init_waitqueue_entry(&wait, tsk);
	tsk->state = TASK_INTERRUPTIBLE;
	add_wait_queue(&sem->wait, &wait);

	/*
	 * Ok, we're set up.  sem->count is known to be less than zero
	 * so we must wait.
	 *
	 * We can let go the lock for purposes of waiting.
	 * We re-acquire it after awaking so as to protect
	 * all semaphore operations.
	 *
	 * If "up()" is called before we call waking_non_zero() then
	 * we will catch it right away.  If it is called later then
	 * we will have to go through a wakeup cycle to catch it.
	 *
	 * Multiple waiters contend for the semaphore lock to see
	 * who gets to gate through and who has to wait some more.
	 */
	for (;;) {
		ret = waking_non_zero_interruptible(sem, tsk);
		if (ret) {
			if (ret == 1)
				/* ret != 0 only if we get interrupted -arca */
				ret = 0;
			break;
		}
		schedule();
		tsk->state = TASK_INTERRUPTIBLE;
	}
	tsk->state = TASK_RUNNING;
	remove_wait_queue(&sem->wait, &wait);
	return ret;
}

EXPORT_SYMBOL(__down_interruptible);

int __down_trylock(struct semaphore * sem)
{
	return waking_non_zero_trylock(sem);
}

EXPORT_SYMBOL(__down_trylock);
