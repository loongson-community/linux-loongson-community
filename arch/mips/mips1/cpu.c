/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 *  Copyright (C) 1996  Ralf Baechle
 */
#include <linux/sched.h>

#include <asm/cache.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/processor.h>

extern asmlinkage void mips1_cacheflush(void *addr, int nbytes, unsigned int flags);

void (*mips_cache_init)(void);

static void
mips1_cache_init(void)
{
	cacheflush = mips1_cacheflush;
}

void (*switch_to_user_mode)(struct pt_regs *regs);

static void
mips1_switch_to_user_mode(struct pt_regs *regs)
{
	regs->cp0_status = regs->cp0_status | ST0_KUC;
}

unsigned long (*thread_saved_pc)(struct thread_struct *t);

/*
 * Return saved PC of a blocked thread.
 */
static unsigned long mips1_thread_saved_pc(struct thread_struct *t)
{
	return ((unsigned long *)(unsigned long)t->reg29)[13];
}

unsigned long (*get_wchan)(struct task_struct *p);

static unsigned long mips1_get_wchan(struct task_struct *p)
{
	/*
	 * This one depends on the frame size of schedule().  Do a
	 * "disass schedule" in gdb to find the frame size.  Also, the
	 * code assumes that sleep_on() follows immediately after
	 * interruptible_sleep_on() and that add_timer() follows
	 * immediately after interruptible_sleep().  Ugly, isn't it?
	 * Maybe adding a wchan field to task_struct would be better,
	 * after all...
	 */
	unsigned long schedule_frame;
	unsigned long pc;

	pc = thread_saved_pc(&p->tss);
	if (pc >= (unsigned long) interruptible_sleep_on && pc < (unsigned long) add_timer) {
		schedule_frame = ((unsigned long *)(long)p->tss.reg30)[13];
		return ((unsigned long *)schedule_frame)[11];
	}
	return pc;
}

void (*pgd_init)(unsigned long page);
void (*copy_page)(unsigned long to, unsigned long from);
asmlinkage void (*restore_fp_context)(struct sigcontext *sc);
asmlinkage void (*save_fp_context)(struct sigcontext *sc);

void
mips1_cpu_init(void)
{
	extern void mips1_cache_init(void);
	extern void mips1_pgd_init(unsigned long page);
	extern void mips1_clear_page(unsigned long page);
	extern void mips1_copy_page(unsigned long to, unsigned long from);
	extern asmlinkage void mips1_restore_fp_context(struct sigcontext *sc);
	extern asmlinkage void mips1_save_fp_context(struct sigcontext *sc);

	mips_cache_init = mips1_cache_init;
	pgd_init = mips1_pgd_init;
	switch_to_user_mode = mips1_switch_to_user_mode;
	thread_saved_pc = mips1_thread_saved_pc;
	get_wchan = mips1_get_wchan;
	clear_page = mips1_clear_page;
	copy_page = mips1_copy_page;
	restore_fp_context = mips1_restore_fp_context;
	save_fp_context = mips1_save_fp_context;
}
