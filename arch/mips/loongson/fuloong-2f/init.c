/*
 * board specific init routines
 *
 * Copyright (C) 2009 Lemote Inc. & Insititute of Computing Technology
 * Author: Wu Zhangjin, wuzj@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/bootmem.h>

#include <asm/bootinfo.h>

void __init mach_prom_init_cmdline(void)
{
	if ((strstr(arcs_cmdline, "ide_core.ignore_cable=")) == NULL)
		strcat(arcs_cmdline, " ide_core.ignore_cable=0");

#ifdef CONFIG_LEMOTE_LYNLOONG
	if ((strstr(arcs_cmdline, "video=")) == NULL)
		strcat(arcs_cmdline, " video=sisfb:1360x768-16@60");
#endif
}
