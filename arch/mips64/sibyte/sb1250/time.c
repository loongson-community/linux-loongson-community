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

/* 
 * These are routines to set up and handle interrupts from the
 * sb1250 general purpose timer 0.  We're using the timer as a
 * system clock, so we set it up to run at 100 Hz.  On every
 * interrupt, we update our idea of what the time of day is, 
 * then call do_timer() in the architecture-independent kernel
 * code to do general bookkeeping (e.g. update jiffies, run
 * bottom halves, etc.)
 */

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>

#include <asm/irq.h>
#include <asm/ptrace.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_scd.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/sibyte/64bit.h>
#include <asm/pgtable.h>


void timer_interrupt(int irq, void *dev_id, struct pt_regs *regs);

void sb1250_time_init(void)
{
	int cpu;
	cpu = smp_processor_id();
	/* Only have 4 general purpose timers */
	if (cpu > 3) {
		BUG();
	}

	sb1250_mask_irq(cpu, 2 + cpu);
	
	/* Map the timer interrupt to ip[4] of this cpu */
	out64(K_INT_MAP_I2, KSEG1 + A_IMR_REGISTER(cpu, R_IMR_INTERRUPT_MAP_BASE) 
	      + ((K_INT_TIMER_0 + cpu)<<3));

	/* the general purpose timer ticks at 1 Mhz independent if the rest of the system */
	/* Disable the timer and set up the count */
	out64(0, KSEG1 + A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));
	out64(1000000/HZ, KSEG1 + A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_INIT));

	/* Set the timer running */
	out64(M_SCD_TIMER_ENABLE|M_SCD_TIMER_MODE_CONTINUOUS,
	      KSEG1 + A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));

	sb1250_unmask_irq(cpu, 2 + cpu);
	/* This interrupt is "special" in that it doesn't use the request_irq way
	   to hook the irq line.  The timer interrupt is initialized early enough
	   to make this a major pain, and it's also firing enough to warrant a
	   bit of special case code.  sb1250_timer_interrupt is called directly
	   from irq_handler.S when IP[4] is set during an interrupt */
}

void sb1250_timer_interrupt(struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	
	/* Reset the timer */
	out64(M_SCD_TIMER_ENABLE|M_SCD_TIMER_MODE_CONTINUOUS,
	      KSEG1 + A_SCD_TIMER_REGISTER(cpu, R_SCD_TIMER_CFG));  
	
	/* Need to do some stuff here with xtime, too, but that looks like
	   it should be architecture independent...does it really belong here? */
	if (!cpu) {
		do_timer(regs);
	} 
	
#ifdef CONFIG_SMP
	{
		int user = user_mode(regs);

		/*
		 * update_process_times() expects us to have done irq_enter().
		 * Besides, if we don't timer interrupts ignore the global
		 * interrupt lock, which is the WrongThing (tm) to do.
		 * Picked from i386 code.
		 */
		irq_enter(cpu, 0);
		update_process_times(user);
		irq_exit(cpu, 0);
	}
#endif /* CONFIG_SMP */
}

