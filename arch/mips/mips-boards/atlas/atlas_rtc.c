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
 * RTC routines for Atlas style attached Dallas chip.
 *
 */
#include <linux/mc146818rtc.h>
#include <asm/mips-boards/atlas.h>

static unsigned char atlas_rtc_read_data(unsigned long addr)
{
	{
		static int done = 0;
		if (!done) {
			prom_printf ("atlas_rtc_read_data, addr=%lx port=%lx\n", addr, RTC_PORT(0));
			prom_printf ("mips_io_port_base=%lx\n", mips_io_port_base);
			done = 1;
		}
	}
	outb(addr, RTC_PORT(0));
	return inb(RTC_PORT(1));
}

static void atlas_rtc_write_data(unsigned char data, unsigned long addr)
{
	outb(addr, RTC_PORT(0));
	outb(data, RTC_PORT(1));
}

static int atlas_rtc_bcd_mode(void)
{
	return 0;
}

struct rtc_ops atlas_rtc_ops = {
	&atlas_rtc_read_data,
	&atlas_rtc_write_data,
	&atlas_rtc_bcd_mode
};
