/*
 * Hardware info about DEC DECstation 5000/2xx systems (otherwise known
 * as 3max or kn02.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995,1996 by Paul M. Antoine, some code and definitions
 * are by curteousy of Chris Fraser.
 *
 * This file is under construction - you were warned!
 */
#include <asm/segment.h>
#ifndef __ASM_MIPS_PMAX_H 
#define __ASM_MIPS_PMAX_H 

/*
 * The addresses below are virtual address. The mappings are
 * created on startup via wired entries in the tlb.
 */

#define PMAX_LOCAL_IO_SPACE     0xe0000000

/*
 * Motherboard regs (kseg1 addresses)
 */
#define PMAX_SSR_ADDR	KSEG1ADDR(0x1ff00000)	/* system control & status reg */

/*
 * SSR defines
 */
#define PMAX_SSR_LEDMASK	0x0000001000		/* diagnostic LED */

#ifndef __LANGUAGE_ASSEMBLY__

extern __inline__ void pmax_set_led(unsigned int bits)
{
	volatile unsigned int *led_register = (unsigned int *) PMAX_SSR_ADDR;

	*led_register = bits & PMAX_SSR_LEDMASK;
}

#endif

/*
 * Some port addresses...
 * FIXME: these addresses are incomplete and need tidying up!
 */
#define PMAX_RTC_BASE	(KSEG1ADDR(0x1fe80000 + 0x200000)) /* ASIC + SL8 */

#endif /* __ASM_MIPS_PMAX_H */
