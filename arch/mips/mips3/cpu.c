/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 *  Copyright (C) 1996  Ralf Baechle
 */
#include <linux/sched.h>

#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/system.h>

extern asmlinkage void mips3_cacheflush(unsigned long addr, unsigned long nbytes, unsigned int flags);

void (*mips_cache_init)(void);
void (*switch_to_user_mode)(struct pt_regs *regs);

static void
mips3_switch_to_user_mode(struct pt_regs *regs)
{
	regs->cp0_status = (regs->cp0_status & ~(ST0_CU0|ST0_KSU)) | KSU_USER;
}

unsigned long (*thread_saved_pc)(struct thread_struct *t);

/*
 * Return saved PC of a blocked thread.
 */
static unsigned long mips3_thread_saved_pc(struct thread_struct *t)
{
	return ((unsigned long long *)(unsigned long)t->reg29)[11];
}

unsigned long (*get_wchan)(struct task_struct *p);

static unsigned long mips3_get_wchan(struct task_struct *p)
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

void (*pgd_init)(unsigned long page);
void (*copy_page)(unsigned long to, unsigned long from);
asmlinkage void (*restore_fp_context)(struct sigcontext *sc);
asmlinkage void (*save_fp_context)(struct sigcontext *sc);

void
mips3_cpu_init(void)
{
	extern void mips3_cache_init(void);
	extern void mips3_pgd_init_32byte_lines(unsigned long page);
	extern void mips3_pgd_init_16byte_lines(unsigned long page);
	extern void mips3_clear_page_32byte_lines(unsigned long page);
	extern void mips3_clear_page_16byte_lines(unsigned long page);
	extern void mips3_copy_page_32byte_lines(unsigned long to, unsigned long from);
	extern void mips3_copy_page_16byte_lines(unsigned long to, unsigned long from);
	extern void mips3_copy_page(unsigned long to, unsigned long from);
	extern asmlinkage void mips3_restore_fp_context(struct sigcontext *sc);
	extern asmlinkage void mips3_save_fp_context(struct sigcontext *sc);

	mips_cache_init = mips3_cache_init;
	if (read_32bit_cp0_register(CP0_CONFIG) & CONFIG_DB) {
		pgd_init = mips3_pgd_init_32byte_lines;
		clear_page = mips3_clear_page_32byte_lines;
		copy_page = mips3_copy_page_32byte_lines;
	} else {
		pgd_init = mips3_pgd_init_16byte_lines;
		clear_page = mips3_clear_page_16byte_lines;
		copy_page = mips3_copy_page_16byte_lines;
	}
	switch_to_user_mode = mips3_switch_to_user_mode;
	thread_saved_pc = mips3_thread_saved_pc;
	get_wchan = mips3_get_wchan;
	restore_fp_context = mips3_restore_fp_context;
	save_fp_context = mips3_save_fp_context;
}
