/*
 *  linux/arch/mips/philips-hpc/nino/prom.c
 *
 *  Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *  
 *  Early initialization code for the Nino.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/bootinfo.h>

char arcs_cmdline[COMMAND_LINE_SIZE];

#ifdef CONFIG_LL_DEBUG
extern void init_uart(void);

int __init prom_printf(const char * fmt, ...)
{
        extern void serial_outc(char);
        static char buf[1024];
        va_list args;
        char c;
        int i = 0;

        /*
         * Printing messages via serial port
         */
        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        for (i = 0; buf[i] != '\0'; i++) {
                c = buf[i];
                if (c == '\n')
                        serial_outc('\r');
                serial_outc(c);
        }

        return i;
}
#endif

/* Do basic initialization */
void __init prom_init(int argc, char **argv,
		unsigned long magic, int *prom_vec)
{
	int i;

	/*
	 * collect args and prepare cmd_line
	 */
	for (i = 1; i < argc; i++) {
		strcat(arcs_cmdline, argv[i]);
		if (i < (argc - 1))
			strcat(arcs_cmdline, " ");
	}

#ifdef CONFIG_LL_DEBUG
	earlyInitUartPR31700();
#endif

	mips_machgroup = MACH_GROUP_PHILIPS;
	mips_machtype = MACH_PHILIPS_NINO;

	/* Add memory region */
#ifdef CONFIG_NINO_4MB
	add_memory_region(0,  4 << 20, BOOT_MEM_RAM); 
#elif CONFIG_NINO_8MB
	add_memory_region(0,  8 << 20, BOOT_MEM_RAM); 
#elif CONFIG_NINO_16MB
	add_memory_region(0, 16 << 20, BOOT_MEM_RAM); 
#endif
}

void __init prom_free_prom_memory (void)
{
}
