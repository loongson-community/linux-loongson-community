/*
 * init.c: PROM library initialisation code.
 *
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov 
 *
 * $Id: init.c,v 1.1 1999/01/17 03:49:40 ralf Exp $
 */
#include <linux/init.h>
#include <linux/config.h>
#include <asm/bootinfo.h>

char arcs_cmdline[CL_SIZE];

__initfunc(int prom_init(unsigned int mem_upper))
{
	mips_memory_upper = mem_upper;
	mips_machgroup  = MACH_GROUP_UNKNOWN;
	mips_machtype   = MACH_UNKNOWN;
	arcs_cmdline[0] = 0;
	return 0;
}

__initfunc(void prom_fixup_mem_map(unsigned long start, unsigned long end))
{
}

void prom_free_prom_memory (void)
{
}
