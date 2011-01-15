/*
 * Based on Ocelot Linux port, which is
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
 *
 * Copyright 2003 ICT CAS
 * Author: Michael Guo <guoyi@ict.ac.cn>
 *
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/bootinfo.h>

#include <loongson.h>
#include <machine.h>

/* Assume the avg. length of the argument is 15 */
#define ARGV_MAX_ARGS (COMMAND_LINE_SIZE / 15)

/* The popular version is PQ1D27 */
#define EC_VER_STR "EC_VER=PQ1D"
unsigned long ec_version = 27;

#ifdef CONFIG_CMDLINE_OVERRIDE
#define cmdline builtin_cmdline
#else
#define cmdline arcs_cmdline
#endif

static inline void get_ec_version(void)
{
	char *p, ret;

	p = strstr(cmdline, EC_VER_STR);
	if (p) {
		p[strlen(EC_VER_STR) + 2] = '\0';
		ret = strict_strtoul(p + strlen(EC_VER_STR), 10, &ec_version);
	}
}

void __init prom_init_cmdline(void)
{
#ifndef CONFIG_CMDLINE_OVERRIDE
	int prom_argc;
	/* PMON passes arguments in 32bit pointers */
	int *_prom_argv;
	int i;
	long l;
	if (fw_arg0 != 0 && fw_arg1 != 0) {
		/* firmware arguments are initialized in head.S */
		prom_argc = min(fw_arg0, (unsigned long)ARGV_MAX_ARGS);
		_prom_argv = (int *)fw_arg1;
		arcs_cmdline[0] = '\0';
		for (i = 0; i < prom_argc; i++) {
			l = (long)_prom_argv[i];
			if (strlen(arcs_cmdline) + strlen(((char *)l) + 1)
			    >= sizeof(arcs_cmdline))
				break;
			strcat(arcs_cmdline, ((char *)l));
			strcat(arcs_cmdline, " ");
		}
	}

	if ((strstr(arcs_cmdline, "console=")) == NULL)
		strcat(arcs_cmdline, " console=tty");
	if ((strstr(arcs_cmdline, "root=")) == NULL)
		strcat(arcs_cmdline, " root=/dev/sda5");

	prom_init_machtype();

	/* append machine specific command line */
	switch (mips_machtype) {
	case MACH_LEMOTE_LL2F:
		if ((strstr(arcs_cmdline, "video=")) == NULL)
			strcat(arcs_cmdline, " video=sisfb:1360x768-16@60");
		break;
	case MACH_LEMOTE_FL2F:
		if ((strstr(arcs_cmdline, "ide_core.ignore_cable=")) == NULL)
			strcat(arcs_cmdline, " ide_core.ignore_cable=0");
		break;
	case MACH_LEMOTE_ML2F7:
		/* Mengloong-2F has a 800x480 screen */
		if ((strstr(arcs_cmdline, "vga=")) == NULL)
			strcat(arcs_cmdline, " vga=0x313");
		break;
	default:
		break;
	}
#endif	/* CONFIG_CMDLINE_OVERRIDE */

#ifdef CONFIG_LEMOTE_YEELOONG2F
	/* The EC version info will be used by the YeeLoong platform driver */
	get_ec_version();
#endif
}
