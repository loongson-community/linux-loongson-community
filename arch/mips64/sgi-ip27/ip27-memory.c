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

#include <asm/page.h>
#include <asm/bootinfo.h>
#include <asm/sn/klconfig.h>

void __init
prom_meminit(void)
{
	unsigned long mb;
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

	if (bank != MD_MEM_BANKS && size != 0)
		printk("Warning: noncontiguous memory configuration, "
		       "not using entire available memory.");

	printk("Found %ldmb of memory.\n", mb);
	mips_memory_upper = PAGE_OFFSET + (mb << 20);
}

/* Called from mem_init() to fixup the mem_map page settings. */
void __init
prom_fixup_mem_map(unsigned long start, unsigned long end)
{
	/* mem_map is already completly setup.  */
}

void __init
prom_free_prom_memory (void)
{
	/* We got nothing to free here ...  */
}
