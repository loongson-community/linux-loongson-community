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

extern asmlinkage void mips4_cacheflush(void *addr, int nbytes, unsigned int flags);

unsigned long page_colour_mask;

void (*mips_cache_init)(void);

static void
mips4_cache_init(void)
{
	/*
	 * The R10000 is in most aspects similar to the R4400.  It
	 * should get some special optimizations.
	 */
	write_32bit_cp0_register(CP0_FRAMEMASK, 0);
	set_cp0_status(ST0_XX, ST0_XX);
	/*
	 * Actually this mask stands for only 16k cache.  This is
	 * correct since the R10000 has multiple ways in it's cache.
	 */
	page_colour_mask = 0x3000;
	cacheflush = mips4_cacheflush;
	/*
	 * The R10k might even work for Linux/MIPS - but we're paranoid
	 * and refuse to run until this is tested on real silicon
	 */
	panic("CPU too expensive - making holiday in the ANDES!");
}

void (*switch_to_user_mode)(struct pt_regs *regs);

static void
mips4_switch_to_user_mode(struct pt_regs *regs)
{
	regs->cp0_status = (regs->cp0_status & ~(ST0_CU0|ST0_KSU)) | KSU_USER;
}

unsigned long (*thread_saved_pc)(struct thread_struct *t);

/*
 * Return saved PC of a blocked thread.
 * XXX This works only for 64 bit kernels.
 */
static unsigned long mips4_thread_saved_pc(struct thread_struct *t)
{
	return ((unsigned long long *)(unsigned long)t->reg29)[11];
}

unsigned long (*get_wchan)(struct task_struct *p);

static unsigned long mips4_get_wchan(struct task_struct *p)
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
		schedule_frame = ((unsigned long long *)(long)p->tss.reg30)[10];
		return (unsigned long)((unsigned long long *)schedule_frame)[9];
	}
	return pc;
}

extern void mips4_clear_page(unsigned long page);
extern void mips4_copy_page(unsigned long to, unsigned long from);
asmlinkage void (*restore_fp_context)(struct sigcontext *sc);
asmlinkage void (*save_fp_context)(struct sigcontext *sc);

void
mips4_cpu_init(void)
{
	extern void mips4_pgd_init(unsigned long page);
	extern asmlinkage void mips4_restore_fp_context(struct sigcontext *sc);
	extern asmlinkage void mips4_save_fp_context(struct sigcontext *sc);

	mips_cache_init = mips4_cache_init;
	pgd_init = mips1_pgd_init;
	switch_to_user_mode = mips4_switch_to_user_mode;
	thread_saved_pc = mips4_thread_saved_pc;
	get_wchan = mips4_get_wchan;
	clear_page = mips4_clear_page;
        restore_fp_context = mips4_restore_fp_context;
        save_fp_context = mips4_save_fp_context;
}
