/*
 *  linux/arch/mips/sgi/kernel/process.c
 *
 *  Reset a SGI.
 */
#include <asm/io.h>
#include <asm/system.h>

void
sgi_hard_reset_now(void)
{
        for(;;)
                prom_imode();
}
