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

#include <asm/sibyte/64bit.h>
#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/addrspace.h>
#include <asm/smp.h>
#include <asm/processor.h>
#include <asm/delay.h>
#include <linux/sched.h>

extern struct cpuinfo_mips cpu_data[NR_CPUS];

/*
 * These are routines for dealing with the sb1250 smp capabilities
 * independent of board/firmware
 */

static u64 mailbox_set_regs[] = 
{  KSEG1 + A_IMR_CPU0_BASE + R_IMR_MAILBOX_SET_CPU, 
   KSEG1 + A_IMR_CPU1_BASE + R_IMR_MAILBOX_SET_CPU  };

static u64 mailbox_clear_regs[] = 
{  KSEG1 + A_IMR_CPU0_BASE + R_IMR_MAILBOX_CLR_CPU, 
   KSEG1 + A_IMR_CPU1_BASE + R_IMR_MAILBOX_CLR_CPU  };

static u64 mailbox_regs[] = 
{  KSEG1 + A_IMR_CPU0_BASE + R_IMR_MAILBOX_CPU, 
   KSEG1 + A_IMR_CPU1_BASE + R_IMR_MAILBOX_CPU  };


/* Simple enough; everything is set up, so just poke the appropriate mailbox
   register, and we should be set */
void sys_send_intercpu_int(int cpu, unsigned int action)
{
#ifdef PARANOID
	if (action & ~0xff) {
		BUG();
	}
#endif
	out64((((u64)action)<< 48), mailbox_set_regs[cpu]);
}


void sb1250_smp_finish(void)
{
	void sb1_sanitize_tlb(void);
	cpu_data[0].udelay_val = loops_per_jiffy;
	cpu_data[1].udelay_val = loops_per_jiffy;
	sb1250_time_init();
	sb1_sanitize_tlb();
}

void sb1250_mailbox_interrupt(struct pt_regs *regs)
{
	/* Function pointer to latch the value before we say we're started */
	unsigned int action;

	/* Load the mailbox register to figure out what we're supposed to do */
	action = (in64(mailbox_regs[smp_processor_id()]) >> 48) & 0xffff;
	
	/* Clear the mailbox to clear the interrupt */
	out64(((u64)0xffff)<<48, mailbox_clear_regs[smp_processor_id()]);
	
	if (action & SMP_INT_RESCHEDULE) {
		current->work.need_resched = 1;
	}
	if (action & SMP_INT_CALL_FUNC) {
		smp_call_function_interrupt();
	}
}

