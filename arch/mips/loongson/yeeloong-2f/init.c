/*
 * board specific init routines
 *
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
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

#include <cs5536/cs5536.h>

void __init mach_prom_init_cmdline(void)
{
	/* set lpc irq to quiet mode */
	_wrmsr(DIVIL_MSR_REG(DIVIL_LEG_IO), 0x00, 0x16000003);

	/*Emulate post for usb */
	_wrmsr(USB_MSR_REG(USB_CONFIG), 0x4, 0xBF000);

	if ((strstr(arcs_cmdline, "no_auto_cmd")) == NULL) {
		unsigned char default_root[50] = "/dev/hda1";
		char *pmon_ver, *ec_ver, *p, version[60], ec_version[64];

		p = arcs_cmdline;

		pmon_ver = strstr(arcs_cmdline, "PMON_VER");
		if (pmon_ver) {
			p = strstr(pmon_ver, " ");
			if (p)
				*p++ = '\0';
			strncpy(version, pmon_ver, 60);
		} else
			strncpy(version, "PMON_VER=Unknown", 60);

		ec_ver = strstr(p, "EC_VER");
		if (ec_ver) {
			p = strstr(ec_ver, " ");
			if (p)
				*p = '\0';
			strncpy(ec_version, ec_ver, 64);
		} else
			strncpy(ec_version, "EC_VER=Unknown", 64);

		p = strstr(arcs_cmdline, "root=");
		if (p) {
			strncpy(default_root, p, sizeof(default_root));
			p = strstr(default_root, " ");
			if (p)
				*p = '\0';
		}

		memset(arcs_cmdline, 0, sizeof(arcs_cmdline));
		strcat(arcs_cmdline, version);
		strcat(arcs_cmdline, " ");
		strcat(arcs_cmdline, ec_version);
		strcat(arcs_cmdline, " ");
		strcat(arcs_cmdline, default_root);
		strcat(arcs_cmdline, " console=tty2");
		strcat(arcs_cmdline, " quiet");
	}
}
