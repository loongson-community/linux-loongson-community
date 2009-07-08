/*
 * Copyright (C) 2009 Lemote, Inc. & Institute of Computing Technology
 * Author: Wu Zhangjin <wuzj@lemote.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

/* machine-specific command line initialization */
#ifdef CONFIG_SYS_HAS_MACH_PROM_INIT_CMDLINE
extern void __init mach_prom_init_cmdline(void);
#else
void __init mach_prom_init_cmdline(void)
{
}
#endif

