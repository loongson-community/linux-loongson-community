/*
 *  arch/mips/ddb5074/prom.c -- NEC DDB Vrc-5074 PROM routines
 *
 *  Copyright (C) 2000 Geert Uytterhoeven <geert@sonycom.com>
 *                     Sony Suprastructure Center Europe (SUPC-E), Brussels
 *
 *  $Id$
 */

#include <linux/init.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>


char arcs_cmdline[CL_SIZE];

void __init prom_init(const char *s)
{
    int i = 0;

//  _serinit();

    if (s != (void *)-1)
	while (*s && i < sizeof(arcs_cmdline)-1)
	    arcs_cmdline[i++] = *s++;
    arcs_cmdline[i] = '\0';

    mips_machgroup = MACH_GROUP_NEC_DDB;
    mips_machtype = MACH_NEC_DDB5074;
    /* 64 MB non-upgradable */
    mips_memory_upper = KSEG0+64*1024*1024;
}

void __init prom_fixup_mem_map(unsigned long start, unsigned long end)
{
}

void __init prom_free_prom_memory(void)
{
}
