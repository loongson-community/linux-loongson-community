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
 * Putting things on the screen/serial line using YAMONs facilities.
 *
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <asm/addrspace.h>
#include <asm/galileo-boards/generic.h>

static char ppbuf[1024];

void (*prom_print_str)(unsigned int out, char *s, int len);

void __init setup_prom_printf(void)
{
        prom_print_str = (void *)*(unsigned int *)YAMON_PROM_PRINT_ADDR;
}

void __init prom_printf(char *fmt, ...)
{
        va_list args;
	int len;

        va_start(args, fmt);
        vsprintf(ppbuf, fmt, args);
	len = strlen(ppbuf);

	prom_print_str(0, ppbuf, len);

        va_end(args);
        return;
}
