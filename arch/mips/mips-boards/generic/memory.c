/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * PROM library functions for acquiring/using memory descriptors given to 
 * us from the YAMON.
 * 
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/page.h>

#include <asm/mips-boards/prom.h>

/* #define DEBUG */

enum yamon_memtypes {
	yamon_dontuse,
	yamon_prom,
        yamon_free,
};
struct prom_pmemblock mdesc[PROM_MAX_PMEMBLOCKS];

#define MEMTYPE_DONTUSE 0
#define MEMTYPE_PROM    1
#define MEMTYPE_FREE    2

#ifdef DEBUG
static char *mtypes[3] = {
        "Dont use memory",
	"YAMON PROM memory",
	"Free memmory",
};
#endif

struct prom_pmemblock * __init prom_getmdesc(void)
{
        char *memsize_str;
	unsigned int memsize;
	
	memsize_str = prom_getenv("memsize");
	if (!memsize_str) {
	        prom_printf("memsize not set in boot prom, set to default (32Mb)\n");
		memsize = 0x02000000;
	} 
	else 
	{
#ifdef DEBUG
	        prom_printf("prom_memsize = %s\n", memsize_str);
#endif
	        memsize = simple_strtol(memsize_str, NULL, 0);
	}

        memset(mdesc, 0, sizeof(mdesc));

	mdesc[0].type = yamon_dontuse;
        mdesc[0].base = 0x00000000;
	mdesc[0].size = 0x00001000;

	mdesc[1].type = yamon_prom;
        mdesc[1].base = 0x00001000;
	mdesc[1].size = 0x000ef000;

#if (CONFIG_MIPS_MALTA)
	/* 
	 * The area 0x000f0000-0x000fffff is allocated for BIOS memory by the
	 * south bridge and PCI access always forwarded to the ISA Bus and 
	 * BIOSCS# is always generated.
	 * This mean that this area can't be used as DMA memory for PCI 
	 * devices.
	 */
	mdesc[2].type = yamon_dontuse;
        mdesc[2].base = 0x000f0000;
	mdesc[2].size = 0x00010000;
#else
	mdesc[2].type = yamon_prom;
        mdesc[2].base = 0x000f0000;
	mdesc[2].size = 0x00010000;
#endif

	mdesc[3].type = yamon_free;
        mdesc[3].base = 0x00100000;
	mdesc[3].size = memsize - mdesc[3].base;

        return &mdesc[0];
}

static struct prom_pmemblock prom_pblocks[PROM_MAX_PMEMBLOCKS];

struct prom_pmemblock * __init prom_getpblock_array(void)
{
	return &prom_pblocks[0];
}

static int __init prom_memtype_classify (unsigned int type)
{
        switch (type) {
	  case yamon_free:
	    return MEMTYPE_FREE;
	  case yamon_prom:
	    return MEMTYPE_PROM;
	  default:
	    return MEMTYPE_DONTUSE;
	}
}

static void __init prom_setup_memupper(void)
{
	struct prom_pmemblock *p, *highest;

	for(p = prom_getpblock_array(), highest = 0; p->size != 0; p++) {
		if(p->base == 0xdeadbeef)
			prom_printf("WHEEE, bogus pmemblock\n");
		if(!highest || p->base > highest->base)
			highest = p;
	}
	mips_memory_upper = highest->base + highest->size;
#ifdef DEBUG
	prom_printf("prom_setup_memupper: mips_memory_upper = %08lx\n",
		    mips_memory_upper);
#endif
}

void __init prom_meminit(void)
{
	struct prom_pmemblock *p;
	int totram;
	int i = 0;

#ifdef DEBUG
	p = prom_getmdesc();
	prom_printf("YAMON MEMORY DESCRIPTOR dump:\n");
	while(p->size) {
		prom_printf("[%d,%p]: base<%08lx> size<%08lx> type<%s>\n",
			    i, p, p->base, p->size, memtypes[p->type]);
		p++;
		i++;
	}
#endif
	p = prom_getmdesc();
	totram = 0;
	i = 0;
	while(p->size) {
	    prom_pblocks[i].type = prom_memtype_classify (p->type);
	    prom_pblocks[i].base = p->base | 0x80000000;
	    prom_pblocks[i].size = p->size;
	    switch (prom_pblocks[i].type) {
	     case MEMTYPE_FREE:
		totram += prom_pblocks[i].size;
#ifdef DEBUG
		prom_printf("free_chunk[%d]: base=%08lx size=%d\n",
			    i, prom_pblocks[i].base,
			    prom_pblocks[i].size);
#endif
		i++;
		break;
	     case MEMTYPE_PROM:
#ifdef DEBUG
		prom_printf("prom_chunk[%d]: base=%08lx size=%d\n",
			    i, prom_pblocks[i].base,
			    prom_pblocks[i].size);
#endif
		i++;
		break;
	     default:
		break;
	    }
	    p++;
	}
	prom_pblocks[i].base = 0xdeadbeef;
	prom_pblocks[i].size = 0; /* indicates last elem. of array */
	printk("PROMLIB: Total free ram %d bytes (%dK,%dMB)\n",
		    totram, (totram/1024), (totram/1024/1024));

	/* Setup upper physical memory bound. */
	prom_setup_memupper();
}

/* Called from mem_init() to fixup the mem_map page settings. */
void __init prom_fixup_mem_map(unsigned long start, unsigned long end)
{
	struct prom_pmemblock *p;
	int i, nents;

	/* Determine number of pblockarray entries. */
	p = prom_getpblock_array();
	for(i = 0; p[i].size; i++)
		;
	nents = i;
restart:
	while(start < end) {
		for(i = 0; i < nents; i++) {
			if((p[i].type == MEMTYPE_FREE) &&
			   (start >= (p[i].base)) &&
			   (start < (p[i].base + p[i].size))) {
				start = p[i].base + p[i].size;
				start &= PAGE_MASK;
				goto restart;
			}
		}
		set_bit(PG_reserved, &mem_map[MAP_NR(start)].flags);
		start += PAGE_SIZE;
	}
}

void prom_free_prom_memory (void)
{
	struct prom_pmemblock *p;
	unsigned long addr;
	unsigned long num_pages = 0;

	for (p = prom_getpblock_array(); p->size != 0; p++) {
		if (p->type != MEMTYPE_PROM)
			continue;

		for (addr = p->base; addr < p->base + p->size;
		     addr += PAGE_SIZE) {
			mem_map[MAP_NR(addr)].flags &= ~(1 << PG_reserved);
			atomic_set(&mem_map[MAP_NR(addr)].count, 1);
			free_page(addr);
			num_pages++;
		}
	}
	printk("Freeing prom memory: %ldk freed\n",
	       (num_pages << (PAGE_SHIFT - 10)));
}
