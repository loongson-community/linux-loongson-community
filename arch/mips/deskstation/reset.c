/*
 * linux/arch/mips/deskstation/process.c
 *
 * Reset a Deskstation.
 */
#include <asm/io.h>
#include <asm/system.h>

void
jazz_hard_reset_now(void)
{
	printk("Implement jazz_hard_reset_now().\n");
	printk("Press reset to continue.\n");
	while(1);
}
