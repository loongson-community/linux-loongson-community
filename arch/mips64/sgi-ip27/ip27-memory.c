/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 by Ralf Baechle
 * Copyright (C) 2000 by Silicon Graphics, Inc.
 *
 * On SGI IP27 the ARC memory configuration data is completly bogus but
 * alternate easier to use mechanisms are available.
 */
#include <linux/init.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

#include <asm/page.h>
#include <asm/bootinfo.h>
#include <asm/sn/klconfig.h>

extern char _end;

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

void __init
prom_meminit(void)
{
	unsigned long free_start, free_end, start_pfn, mb, bootmap_size;
	int bank, size;
	lboard_t *board;
	klmembnk_t *mem;

	board = find_lboard(KLTYPE_IP27);
	if (!board)
		panic("Can't find memory info for myself.");

	mem = (klmembnk_t *) KLCF_COMP(board, IP27_MEM_INDEX);
	if (!mem)
		panic("I'm running but don't exist?");

	mb = 0;
	for (bank = 0; bank < MD_MEM_BANKS; bank++) {
		size = KLCONFIG_MEMBNK_SIZE(mem, bank);
		mb += size;
		if (size != 512) {
			break;
		}
	}

	free_start = PFN_ALIGN(&_end) - (CKSEG0 - K0BASE);
	free_end = K0BASE + (mb << 20);
	start_pfn = PFN_UP((unsigned long)&_end - CKSEG0);

	if (bank != MD_MEM_BANKS && size != 0)
		printk("Warning: noncontiguous memory configuration, "
		       "not using entire available memory.");

	/* Register all the contiguous memory with the bootmem allocator
	   and free it.  Be careful about the bootmem freemap.  */
	bootmap_size = init_bootmem(start_pfn, mb << (20 - PAGE_SHIFT));
	free_bootmem(__pa(free_start), (mb << 20) - __pa(free_start));
	reserve_bootmem(__pa(free_start), bootmap_size);
	free_bootmem(0x19000, 0x1c000 - 0x19000);

	printk("Found %ldmb of memory.\n", mb);
}

int __init page_is_ram(unsigned long pagenr)
{
        return 1;
}

void __init
prom_free_prom_memory (void)
{
	/* We got nothing to free here ...  */
}
