/*
 *  linux/kernel/panic.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 * This function is used through-out the kernel (including mm and fs)
 * to indicate a major problem.
 */
#include <stdarg.h>

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <asm/sgialib.h>

asmlinkage void sys_sync(void);	/* it's really int */
extern void do_unblank_screen(void);
extern int C_A_D;

int panic_timeout = 0;

void panic_setup(char *str, int *ints)
{
	if (ints[0] == 1)
		panic_timeout = ints[1];
}

NORET_TYPE void panic(const char * fmt, ...)
{
	static char buf[1024];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	printk(KERN_EMERG "Kernel panic: %s\n",buf);
	if (current == task[0])
		printk(KERN_EMERG "In swapper task - not syncing\n");
	else
		sys_sync();

	do_unblank_screen();

#ifdef CONFIG_SGI
	if (panic_timeout > 0)
	{
		int i;

		/*
	 	 * Delay timeout seconds before rebooting the machine. 
		 * We can't use the "normal" timers since we just panicked..
	 	 */
		prom_printf(KERN_EMERG "Rebooting in %d seconds..",panic_timeout);
		for(i = 0; i < (panic_timeout*1000); i++)
			udelay(1000);
		hard_reset_now();
	}
#if 0
	printk("Hit a key\n");
	prom_getchar();
	romvec->imode();
#endif
#endif
	for(;;);
}

