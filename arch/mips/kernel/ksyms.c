/*
 * Export MIPS-specific functions needed for loadable modules.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#include <linux/module.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <asm/dma.h>
#include <asm/floppy.h>
#include <asm/io.h>

static struct symbol_table arch_symbol_table = {
#include <linux/symtab_begin.h>
	X(EISA_bus),
	/*
	 * String functions
	 */
	X(memset),
	X(memcpy),
	X(memmove),
	X(bcopy),
	/*
	 * Functions to control caches.
	 */
	X(fd_cacheflush),
	/*
	 * Base address of ports for Intel style I/O.
	 */
	X(port_base),
#include <linux/symtab_end.h>
};

void arch_syms_export(void)
{
	register_symtab(&arch_symbol_table);
}
