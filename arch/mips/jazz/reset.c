/*
 *  linux/arch/mips/acn/process.c
 *
 *  Reset a Jazz machine.
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
