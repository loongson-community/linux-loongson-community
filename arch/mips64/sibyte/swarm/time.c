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
 * Time routines for the swarm board.  We pass all the hard stuff
 * through to the sb1250 handling code.  Only thing we really keep
 * track of here is what time of day we think it is.  And we don't
 * really even do a good job of that...
 */


#include <linux/time.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <asm/system.h>
#include <asm/sibyte/sb1250.h>


static unsigned long long sec_bias = 0;
static unsigned int usec_bias = 0;

static rwlock_t time_lock = RW_LOCK_UNLOCKED;

void do_settimeofday(struct timeval *tv)
{
	unsigned long saved_jiffies;
	unsigned long flags;
	saved_jiffies = jiffies;
	write_lock_irqsave(&time_lock, flags);
	sec_bias = (saved_jiffies/HZ) - tv->tv_sec;
	usec_bias = ((saved_jiffies%HZ)*(1000000/HZ)) - tv->tv_usec;
	write_unlock_irqrestore(&time_lock, flags);
}

void do_gettimeofday(struct timeval *tv)
{
	unsigned long saved_jiffies;
	unsigned long flags;
	saved_jiffies = jiffies;
	read_lock_irqsave(&time_lock, flags);
	tv->tv_sec = sec_bias + (saved_jiffies/HZ);
	tv->tv_usec = usec_bias + ((saved_jiffies%HZ) * (1000000/HZ));
	read_unlock_irqrestore(&time_lock, flags);
}


/*
 *  Bring up the timer at 100 Hz.  
 */
void time_init(void)
{
	/* Just use the sb1250 scd timer.  This is a pass through because other
	   boards may want to use a different timer... */
	sb1250_time_init();
}
