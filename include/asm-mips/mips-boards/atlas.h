/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Defines of the Atlas board specific address-MAP, registers, etc.
 *
 */
#ifndef _MIPS_ATLAS_H
#define _MIPS_ATLAS_H

#include <asm/addrspace.h>

/* 
 * Atlas RTC-device indirect register access.
 */
#define ATLAS_RTC_ADR_REG       (KSEG1ADDR(0x1f000800))
#define ATLAS_RTC_DAT_REG       (KSEG1ADDR(0x1f000808))


/*
 * Atlas interrupt controller register base.
 */
#define ATLAS_ICTRL_REGS_BASE   (KSEG1ADDR(0x1f000000))

/*
 * Atlas UART register base.
 */
#define ATLAS_UART_REGS_BASE    (KSEG1ADDR(0x1f000900))
#define ATLAS_BASE_BAUD ( 3686400 / 16 ) 

/*
 * Atlas PSU standby register.
 */
#define ATLAS_PSUSTBY_REG       (KSEG1ADDR(0x1f000600))
#define ATLAS_GOSTBY            0x4d


#endif /* !(_MIPS_ATLAS_H) */
