/*
 *  arch/mips/ddb5476/prom.c -- NEC DDB Vrc-5476 PROM routines
 *
 *  Copyright (C) 2000 Geert Uytterhoeven <geert@sonycom.com>
 *                     Sony Software Development Center Europe (SDCE), Brussels
 *
 *   	Jun Sun - modified for DDB5476.
 *
 *  $Id: prom.c,v 1.1 2000/01/26 00:07:44 ralf Exp $
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bootmem.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>


char arcs_cmdline[CL_SIZE];

extern char _end;

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)


/* [jsun@junsun.net] PMON passes arguments in C main() style */
void __init prom_init(int argc, const char **arg)
{
    int i;
    unsigned long mem_size, free_start, free_end, start_pfn, bootmap_size;

    /* arg[0] is "g", the rest is boot parameters */
    arcs_cmdline[0] = '\0';
    for (i=1; i< argc; i++) {
	if (strlen(arcs_cmdline) + strlen(arg[i] + 1) >= CL_SIZE) break;
	strcat(arcs_cmdline, arg[i]);
	strcat(arcs_cmdline, " ");
    }

    mips_machgroup = MACH_GROUP_NEC_DDB;
    mips_machtype = MACH_NEC_DDB5476;
    /* 64 MB non-upgradable */
    mem_size = 64 << 20;

    free_start = PHYSADDR(PFN_ALIGN(&_end));
    free_end = mem_size;
    start_pfn = PFN_UP((unsigned long)&_end);

    /* Register all the contiguous memory with the bootmem allocator
       and free it.  Be careful about the bootmem freemap.  */
    bootmap_size = init_bootmem(start_pfn, mem_size >> PAGE_SHIFT);

    /* Free the entire available memory after the _end symbol.  */
    free_start += bootmap_size;
    free_bootmem(free_start, free_end-free_start);
}

void __init prom_fixup_mem_map(unsigned long start, unsigned long end)
{
}

void __init prom_free_prom_memory(void)
{
}
