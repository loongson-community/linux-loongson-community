/*
 * Hardware info about DEC Personal DECStation systems (otherwise known
 * as maxine (internal DEC codename).
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

#ifndef __ASM_DEC_MAXINE_H 
#define __ASM_DEC_MAXINE_H 

/*
 * The addresses below are virtual address. The mappings are
 * created on startup via wired entries in the tlb.
 */

#define PMAX_LOCAL_IO_SPACE     0xe0000000

/*
 * Motherboard regs (kseg1 addresses)
 */
#define PMAX_SSR_ADDR		KSEG1ADDR(0x1c040100)	/* system support reg */

/*
 * SSR defines
 */
#define PMAX_SSR_LEDMASK	0x00000001		/* power LED */

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
#define PMAX_RTC_BASE	(KSEG1ADDR(0x1c000000 + 0x200000)) /* ASIC + SL8 */

#endif /* __ASM_DEC_MAXINE_H */
