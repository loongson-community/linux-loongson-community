/*
 * identify.c: machine identification code.
 *
 * Copyright (C) 1998 Harald Koerfgen and Paul M. Antoine
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>

#include <asm/bootinfo.h>
#include <asm/dec/prom.h>

#include "dectypes.h"

extern unsigned long mips_machgroup;
extern unsigned long mips_machtype;

static const char *dec_system_strings[] = {
	[MACH_DSUNKNOWN]	"unknown DECstation",
	[MACH_DS23100]		"DECstation 2100/3100",
	[MACH_DS5100]		"DECstation 5100",
	[MACH_DS5000_200]	"DECstation 5000/200",
	[MACH_DS5000_1XX]	"DECstation 5000/1xx",
	[MACH_DS5000_XX]	"Personal DECstation 5000/xx",
	[MACH_DS5000_2X0]	"DECstation 5000/2x0",
	[MACH_DS5400]		"DECstation 5400",
	[MACH_DS5500]		"DECstation 5500",
	[MACH_DS5800]		"DECstation 5800"
};

const char *get_system_type(void)
{
#define STR_BUF_LEN	64
	static char system[STR_BUF_LEN];
	static int called = 0;

	if (called == 0) {
		called = 1;
		snprintf(system, STR_BUF_LEN, "Digital %s",
			 dec_system_strings[mips_machtype]);
	}

	return system;
}

void __init prom_identify_arch(u32 magic)
{
	unsigned char dec_cpunum, dec_firmrev, dec_etc, dec_systype;
	u32 dec_sysid;

	if (!prom_is_rex(magic)) {
		dec_sysid = simple_strtoul(prom_getenv("systype"), (char **)0, 0);
	} else {
		dec_sysid = rex_getsysid();
		if (dec_sysid == 0) {
			prom_printf("Zero sysid returned from PROMs! Assuming PMAX-like machine.\n");
			dec_sysid = 1;
		}
	}

	dec_cpunum = (dec_sysid & 0xff000000) >> 24;
	dec_systype = (dec_sysid & 0xff0000) >> 16;
	dec_firmrev = (dec_sysid & 0xff00) >> 8;
	dec_etc = dec_sysid & 0xff;

	/* We're obviously one of the DEC machines */
	mips_machgroup = MACH_GROUP_DEC;

	/*
	 * FIXME: This may not be an exhaustive list of DECStations/Servers!
	 * Put all model-specific initialisation calls here.
	 */
	switch (dec_systype) {
	case DS2100_3100:
		mips_machtype = MACH_DS23100;
		break;
	case DS5100:		/* DS5100 MIPSMATE */
		mips_machtype = MACH_DS5100;
		break;
	case DS5000_200:	/* DS5000 3max */
		mips_machtype = MACH_DS5000_200;
		break;
	case DS5000_1XX:	/* DS5000/100 3min */
		mips_machtype = MACH_DS5000_1XX;
		break;
	case DS5000_2X0:	/* DS5000/240 3max+ */
		mips_machtype = MACH_DS5000_2X0;
		break;
	case DS5000_XX:	/* Personal DS5000/2x */
		mips_machtype = MACH_DS5000_XX;
		break;
	case DS5800:		/* DS5800 Isis */
		mips_machtype = MACH_DS5800;
		break;
	case DS5400:		/* DS5400 MIPSfair */
		mips_machtype = MACH_DS5400;
		break;
	case DS5500:		/* DS5500 MIPSfair-2 */
		mips_machtype = MACH_DS5500;
		break;
	default:
		mips_machtype = MACH_DSUNKNOWN;
		break;
	}

	if (mips_machtype == MACH_DSUNKNOWN)
		prom_printf("This is an %s, id is %x\n",
			    dec_system_strings[mips_machtype],
			    dec_systype);
	else
		prom_printf("This is a %s\n",
			    dec_system_strings[mips_machtype]);
}
