/*
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzj@lemote.com
 *
 * Copyright (c) 2009 Zhang Le <r0bertz@gentoo.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/errno.h>
#include <asm/bootinfo.h>

#include <loongson.h>
#include <machine.h>

/* please ensure the length of the machtype string is less than 50 */
#define MACHTYPE_LEN 50

static const char *system_types[] = {
	[MACH_LOONGSON_UNKNOWN]         "unknown loongson machine",
	[MACH_LEMOTE_FL2E]              "lemote-fuloong-2e-box",
	[MACH_LEMOTE_FL2F]              "lemote-fuloong-2f-box",
	[MACH_LEMOTE_ML2F7]             "lemote-mengloong-2f-7inches",
	[MACH_LEMOTE_YL2F89]            "lemote-yeeloong-2f-8.9inches",
	[MACH_DEXXON_GDIUM2F10]         "dexxon-gidum-2f-10inches",
	[MACH_LEMOTE_NAS]		"lemote-nas-2f",
	[MACH_LEMOTE_LL2F]              "lemote-lynloong-2f",
	[MACH_LOONGSON_END]             NULL,
};

const char *get_system_type(void)
{
	return system_types[mips_machtype];
}

void __init prom_init_machtype(void)
{
	char *p, str[MACHTYPE_LEN];
	int machtype = MACH_LEMOTE_FL2E;

	mips_machtype = LOONGSON_MACHTYPE;

	p = strstr(arcs_cmdline, "machtype=");
	if (!p) {
#ifdef CONFIG_LEMOTE_MACH2F
		/* We share the same kernel image file among Lemote 2F family
		 * of machines, and provide the machtype= kernel command line
		 * to users to indicate their machine, this command line will
		 * be passed by the latest PMON automatically. and fortunately,
		 * up to now, we can get the machine type from the PMON_VER=
		 * commandline directly except the NAS machine, In the old
		 * machines, this will help the users a lot.
		 *
		 * If no "machtype=" passed, get machine type from "PMON_VER=".
		 * 	PMON_VER=LM8089		Lemote 8.9'' netbook
		 * 	         LM8101		Lemote 10.1'' netbook
		 * 	(The above two netbooks have the same kernel support)
		 *	         LM6XXX		Lemote FuLoong(2F) box series
		 *	         LM9XXX		Lemote LynLoong PC series
		 */
		if (strstr(arcs_cmdline, "PMON_VER=LM")) {
			if (strstr(arcs_cmdline, "PMON_VER=LM8"))
				mips_machtype = MACH_LEMOTE_YL2F89;
			else if (strstr(arcs_cmdline, "PMON_VER=LM6"))
				mips_machtype = MACH_LEMOTE_FL2F;
			else if (strstr(arcs_cmdline, "PMON_VER=LM9"))
				mips_machtype = MACH_LEMOTE_LL2F;
			else
				mips_machtype = MACH_LEMOTE_NAS;

			strcat(arcs_cmdline, " machtype=");
			strcat(arcs_cmdline, system_types[mips_machtype]);
			strcat(arcs_cmdline, " ");
		}
#endif
		return;
	}
	p += strlen("machtype=");
	strncpy(str, p, MACHTYPE_LEN);
	p = strstr(str, " ");
	if (p)
		*p = '\0';

	for (; system_types[machtype]; machtype++)
		if (strstr(system_types[machtype], str)) {
			mips_machtype = machtype;
			break;
		}
}
