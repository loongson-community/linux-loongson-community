#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

/*
 * define DEBUG if you want the wait-queues to have some extra
 * debugging code. It's not normally used, but might catch some
 * wait-queue coding errors.
 *
 *  #define DEBUG
 */

#define HZ 100

/*
 * System setup and hardware bug flags..
 */
extern int hard_math;

#include <linux/binfmts.h>
#include <linux/personality.h>
#include <linux/tasks.h>
#include <asm/system.h>

/*
 * These are the constant used to fake the fixed-point load-average
 * counting. Some notes:
 *  - 11 bit fractions expand to 22 bits by the multiplies: this gives
 *    a load-average precision of 10 bits integer + 11 bits fractional
 *  - if you want to count load-averages more often, you need more
 *    precision, or rounding will get you. With 2-second counting freq,
 *    the EXP_n values would be 1981, 2034 and 2043 if still using only
 *    11 bit fractions.
 */
extern unsigned long avenrun[];         /* Load averages */

#define FSHIFT          11              /* nr of bits of precision */
#define FIXED_1         (1<<FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_FREQ       (5*HZ)          /* 5 sec intervals */
#define EXP_1           1884            /* 1/exp(5sec/1min) as fixed-point */
#define EXP_5           2014            /* 1/exp(5sec/5min) */
#define EXP_15          2037            /* 1/exp(5sec/15min) */

#define CALC_LOAD(load,exp,n) \
	load *= exp; \
	load += n*(FIXED_1-exp); \
	load >>= FSHIFT;

#define CT_TO_SECS(x)   ((x) / HZ)
#define CT_TO_USECS(x)  (((x) % HZ) * 1000000/HZ)

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/signal.h>
#include <linux/time.h>
#include <linux/param.h>
#include <linux/resource.h>
/* #include <linux/vm86.h> */
#include <linux/math_emu.h>
#include <linux/ptrace.h>

#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_ZOMBIE             3
#define TASK_STOPPED            4
#define TASK_SWAPPING           5

#ifndef NULL
#define NULL ((void *) 0)
#endif

#ifdef __KERNEL__

extern void sched_init(void);
extern void show_state(void);
extern void trap_init(void);

asmlinkage void schedule(void);

#endif /* __KERNEL__ */

struct files_struct {
	int count;
	fd_set close_on_exec;
	struct file * fd[NR_OPEN];
};

#define INIT_FILES { \
	0, \
	{ { 0, } }, \
	{ NULL, } \
}

struct fs_struct {
	int count;
	unsigned short umask;
	struct inode * root, * pwd;
};

#define INIT_FS { \
	0, \
	0022, \
	NULL, NULL \
}

struct mm_struct {
	int count;
	unsigned long start_code, end_code, end_data;
	unsigned long start_brk, brk, start_stack, start_mmap;
	unsigned long arg_start, arg_end, env_start, env_end;
	unsigned long rss;
	unsigned long min_flt, maj_flt, cmin_flt, cmaj_flt;
	int swappable:1;
	unsigned long swap_address;
	unsigned long old_maj_flt;      /* old value of maj_flt */
	unsigned long dec_flt;          /* page fault count of the last time */
	unsigned long swap_cnt;         /* number of pages to swap on next pass
*/
	struct vm_area_struct * mmap;
};

#define INIT_MMAP { &init_task, 0, 0x40000000, PAGE_SHARED, }

#define INIT_MM { \
		0, \
		0, 0, 0, \
		0, 0, 0, 0, \
		0, 0, 0, 0, \
		0, \
/* ?_flt */	0, 0, 0, 0, \
		0, \
/* swap */	0, 0, 0, 0, \
		&init_mmap }

/*
 * Per process flags
 */
#define PF_ALIGNWARN    0x00000001	/* Print alignment warning msgs */
					/* Not implemented yet, only for 486*/
#define PF_PTRACED      0x00000010	/* set if ptrace (0) has been called. */
#define PF_TRACESYS     0x00000020	/* tracing system calls */

/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define COPYVM		0x00000100	/* set if VM copy desired (like normal f
ork()) */
#define COPYFD		0x00000200	/* set if fd's should be copied, not sha
red (NI) */

#ifdef __KERNEL__

extern struct task_struct init_task;
extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern unsigned long volatile jiffies;
extern unsigned long itimer_ticks;
extern unsigned long itimer_next;
extern struct timeval xtime;
extern int need_resched;

#define CURRENT_TIME (xtime.tv_sec)

extern void sleep_on(struct wait_queue ** p);
extern void interruptible_sleep_on(struct wait_queue ** p);
extern void wake_up(struct wait_queue ** p);
extern void wake_up_interruptible(struct wait_queue ** p);

extern void notify_parent(struct task_struct * tsk);
extern int send_sig(unsigned long sig,struct task_struct * p,int priv);
extern int in_group_p(gid_t grp);

extern int request_irq(unsigned int irq,void (*handler)(int),
                       unsigned long flags, const char *device);
extern void free_irq(unsigned int irq);

/*
 * The wait-queues are circular lists, and you have to be *very* sure
 * to keep them correct. Use only these two functions to add/remove
 * entries in the queues.
 */
extern inline void add_wait_queue(struct wait_queue ** p,
                                  struct wait_queue * wait)
{
        unsigned long flags;

#if defined (DEBUG) && defined (__mips__)
	/*
	 * FIXME: I don't work for MIPS yet
	 */
	if (wait->next) {
		unsigned long pc;
		__asm__ __volatile__("call 1f\n"
			"1:\tpopl %0":"=r" (pc));
		printk("add_wait_queue (%08x): wait->next = %08x\n",pc,
		       (unsigned long) wait->next);
        }
#endif
	save_flags(flags);
	cli();
	if (!*p) {
		wait->next = wait;
		*p = wait;
	} else {
		wait->next = (*p)->next;
		(*p)->next = wait;
	}
	restore_flags(flags);
}

extern inline void remove_wait_queue(struct wait_queue ** p,
                                     struct wait_queue *wait)
{
	unsigned long flags;
	struct wait_queue * tmp;
#if defined (DEBUG) && !defined(__mips__)
	/*
	 * FIXME: I don't work for MIPS yet
	 */
	unsigned long ok = 0;
#endif

	save_flags(flags);
	cli();
	if ((*p == wait) &&
#if defined (DEBUG) && !defined(__mips__)
	/*
	 * FIXME: I don't work for MIPS yet
	 */
	    (ok = 1) &&
#endif
	    ((*p = wait->next) == wait)) {
		*p = NULL;
		} else {
		tmp = wait;
		while (tmp->next != wait) {
			tmp = tmp->next;
#if defined (DEBUG) && !defined(__mips__)
			/*
			 * FIXME: I don't work for MIPS yet
			 */
			if (tmp == *p)
				ok = 1;
#endif
		}
		tmp->next = wait->next;
	}
	wait->next = NULL;
	restore_flags(flags);
#if defined (DEBUG) && !defined(__mips__)
	/*
	 * FIXME: I don't work for MIPS yet
	 */
	if (!ok) {
		printk("removed wait_queue not on list.\n");
		printk("list = %08x, queue = %08x\n",(unsigned long) p, (unsigne
d long) wait);
		__asm__("call 1f\n1:\tpopl %0":"=r" (ok));
		printk("eip = %08x\n",ok);
	}
#endif
}

extern inline void select_wait(struct wait_queue ** wait_address,
                               select_table *p)
{
	struct select_table_entry * entry;

	if (!p || !wait_address)
		return;
	if (p->nr >= __MAX_SELECT_TABLE_ENTRIES)
		return;
	entry = p->entry + p->nr;
	entry->wait_address = wait_address;
	entry->wait.task = current;
	entry->wait.next = NULL;
	add_wait_queue(wait_address,&entry->wait);
	p->nr++;
}

extern void __down(struct semaphore * sem);

/*
 * These are not yet interrupt-safe
 */
extern inline void down(struct semaphore * sem)
{
	if (sem->count <= 0)
		__down(sem);
	sem->count--;
}

extern inline void up(struct semaphore * sem)
{
	sem->count++;
	wake_up(&sem->wait);
}

#define REMOVE_LINKS(p) do { unsigned long flags; \
	save_flags(flags) ; cli(); \
	(p)->next_task->prev_task = (p)->prev_task; \
	(p)->prev_task->next_task = (p)->next_task; \
	restore_flags(flags); \
	if ((p)->p_osptr) \
		(p)->p_osptr->p_ysptr = (p)->p_ysptr; \
	if ((p)->p_ysptr) \
		(p)->p_ysptr->p_osptr = (p)->p_osptr; \
	else \
		(p)->p_pptr->p_cptr = (p)->p_osptr; \
	} while (0)

#define SET_LINKS(p) do { unsigned long flags; \
	save_flags(flags); cli(); \
	(p)->next_task = &init_task; \
	(p)->prev_task = init_task.prev_task; \
	init_task.prev_task->next_task = (p); \
	init_task.prev_task = (p); \
	restore_flags(flags); \
	(p)->p_ysptr = NULL; \
	if (((p)->p_osptr = (p)->p_pptr->p_cptr) != NULL) \
		(p)->p_osptr->p_ysptr = p; \
	(p)->p_pptr->p_cptr = p; \
	} while (0)

#define for_each_task(p) \
	for (p = &init_task ; (p = p->next_task) != &init_task ; )

#endif /* __KERNEL__ */

/*
 * Include machine dependent stuff
 */
#include <asm/sched.h>

#endif
