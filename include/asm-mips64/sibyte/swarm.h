/*
 * Copyright (C) 2001 Broadcom Corporation
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

#ifndef _ASM_SIBYTE_SWARM_H
#define _ASM_SIBYTE_SWARM_H

#include <asm/addrspace.h>

/*#define IO_SPACE_BASE 0xffffffffa0000000UL*/
#define IO_SPACE_BASE K1BASE

/* Not sure this is right... ---JDC */
#define IO_SPACE_LIMIT 0xffff

#define KERNEL_RESERVED_MEM 0x100000

void swarm_setup(void);

#define LED_BASE_ADDR 0x100a0020
void setleds(char *str); 

#endif
