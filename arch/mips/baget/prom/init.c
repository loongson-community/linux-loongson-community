/*
 * init.c: PROM library initialisation code.
 *
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov 
 */
#include <linux/init.h>
#include <asm/bootinfo.h>

char arcs_cmdline[COMMAND_LINE_SIZE];

int __init prom_init(unsigned int mem_upper)
{
	mips_memory_upper = mem_upper;
	mips_machgroup  = MACH_GROUP_UNKNOWN;
	mips_machtype   = MACH_UNKNOWN;
	arcs_cmdline[0] = 0;
	return 0;
}

void prom_free_prom_memory (void)
{
}
