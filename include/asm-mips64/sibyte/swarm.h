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
#ifndef __ASM_SIBYTE_SWARM_H
#define __ASM_SIBYTE_SWARM_H

#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_int.h>

#define KERNEL_RESERVED_MEM 0x100000

#define LEDS_CS         3

#ifdef CONFIG_SIBYTE_SWARM

/* Generic bus chip selects */
#define IDE_CS          4
#define PCMCIA_CS       6

/* GPIOs */
#define K_GPIO_GB_IDE   4
#define K_INT_GB_IDE    (K_INT_GPIO_0 + K_GPIO_GB_IDE)
#define K_GPIO_PC_READY 9
#define K_INT_PC_READY  (K_INT_GPIO_0 + K_GPIO_PC_READY)

#endif

#ifdef __ASSEMBLY__
#define setleds(t0,t1,c0,c1,c2,c3) \
	li	t0, (LED_BASE_ADDR|0xa0000000); \
	li	t1, c0; \
	sb	t1, 0x18(t0); \
	li	t1, c1; \
	sb	t1, 0x10(t0); \
	li	t1, c2; \
	sb	t1, 0x08(t0); \
	li	t1, c3; \
	sb	t1, 0x00(t0)
#else
void swarm_setup(void);
void setleds(char *str);

#define AT_spin \
	__asm__ __volatile__ (		\
		".set noat\n"		\
		"li $at, 0\n"		\
		"1: beqz $at, 1b\n"	\
		".set at\n"		\
		)
#endif

#endif /* __ASM_SIBYTE_SWARM_H */
