/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999 MIPS Technologies, Inc.  All rights reserved.
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
 * Defines for the Atlas interrupt controller.
 *
 */
#ifndef _MIPS_ATLASINT_H
#define _MIPS_ATLASINT_H

/* Number of IRQ supported on hw interrupt 0. */
#define ATLASINT_UART      0
#define ATLASINT_END      32

/*
 * Atlas registers are memory mapped on 64-bit aligned boundaries and
 * only word access are allowed.
 */
struct atlas_ictrl_regs {
        volatile unsigned int intraw;
        int dummy1;
        volatile unsigned int intseten;
        int dummy2;
        volatile unsigned int intrsten;
        int dummy3;
        volatile unsigned int intenable;
        int dummy4;
        volatile unsigned int intstatus;
        int dummy5;
};

extern void atlasint_init(void);

#endif /* !(_MIPS_ATLASINT_H) */
