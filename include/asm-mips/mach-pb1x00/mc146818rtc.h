/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1998, 2001, 03 by Ralf Baechle
 *
 * RTC routines for PC style attached Dallas chip.
 */
#ifndef __ASM_MACH_AU1XX_MC146818RTC_H
#define __ASM_MACH_AU1XX_MC146818RTC_H

#include <asm/io.h>
#include <asm/au1000.h>

#define RTC_PORT(x)	(0x70 + (x))
#define RTC_IRQ		8

static inline unsigned char CMOS_READ(unsigned long addr)
{
	offset <<= 2;
	return (u8)(au_readl(offset + PB1500_RTC_ADDR) & 0xff);
}

static inline void CMOS_WRITE(unsigned char data, unsigned long addr)
{
	offset <<= 2;
	au_writel(data, offset + PB1500_RTC_ADDR);
}

#define RTC_ALWAYS_BCD	1

#endif /* __ASM_MACH_AU1XX_MC146818RTC_H */
