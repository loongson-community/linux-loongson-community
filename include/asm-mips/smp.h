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

#ifndef __ASM_MIPS_SMP_H
#define __ASM_MIPS_SMP_H

#include <linux/config.h>

#ifdef CONFIG_SMP

#include <asm/spinlock.h>
#include <asm/atomic.h>
#include <asm/current.h>


/* Mappings are straight across.  If we want
   to add support for disabling cpus and such,
   we'll have to do what the mips64 port does here */
#define cpu_logical_map(cpu)	(cpu)
#define cpu_number_map(cpu)     (cpu)

#define smp_processor_id()  (current->processor)


/* I've no idea what the real meaning of this is */
#define PROC_CHANGE_PENALTY	20

#define NO_PROC_ID	(-1)

struct smp_fn_call_struct {
	spinlock_t lock;
	atomic_t   finished;
	void (*fn)(void *);
	void *data;
};

extern struct smp_fn_call_struct smp_fn_call;

#endif /* CONFIG_SMP */
#endif /* __ASM_MIPS_SMP_H */
