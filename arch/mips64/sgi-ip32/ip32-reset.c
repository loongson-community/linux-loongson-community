/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001 Keith M Wesolowski
 * Copyright (C) 2001 Paul Mundt
 */
#include <asm/sgialib.h>

void machine_restart(char *cmd)
{
	ArcReboot();
}

void machine_halt(void)
{
	ArcEnterInteractiveMode();
}

void machine_power_off(void)
{
	machine_halt();
}
