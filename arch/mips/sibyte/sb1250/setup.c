/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_scd.h>
#include <asm/sibyte/64bit.h>

/* Setup code likely to be common to all BCM1250 platforms */

unsigned int sb1250_pass;

void sb1250_setup(void)
{
	int bad_config = 0;

	sb1250_pass = G_SYS_REVISION(in64(IO_SPACE_BASE | A_SCD_SYSTEM_REVISION));
	/* sb1250_pass is more specific than "1", "2" etc.  There are
           many revision numbers corresponding to "Pass 2". */
	switch(sb1250_pass) {
	case 1:
#ifndef CONFIG_SB1_PASS_1_WORKAROUNDS
		printk("@@@@ This is a pass 1 board, and the kernel doesn't have the proper workarounds compiled in. @@@@");
		bad_config = 1;
#endif
		break;
	default:
#if defined(CONFIG_CPU_HAS_PREFETCH) || !defined(CONFIG_SB1_PASS_2_WORKAROUNDS)
		printk("@@@@ This is a pass 2 board, and the kernel doesn't have the proper workarounds compiled in. @@@@");
		printk("@@@@ Prefetches are enabled in this kernel, but are buggy on this board.  @@@@");
		bad_config = 1;
#endif
		break;
	}
	/* XXXKW this is too early for panic/printk to actually do much! */
	if (bad_config) {
		panic("Invalid configuration for this pass.");
	}
}
