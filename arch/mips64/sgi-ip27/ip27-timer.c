/* $Id$
 *
 * Copytight (C) 1999 Ralf Baechle (ralf@gnu.org)
 * Copytight (C) 1999 Silicon Graphics, Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/param.h>
#include <linux/timex.h>
#include <linux/mm.h>		

#include <asm/pgtable.h>
#include <asm/sgialib.h>
#include <asm/sn/arch.h>
#include <asm/sn/addrs.h>
#include <asm/sn/sn0/ip27.h>
#include <asm/sn/sn0/hub.h>

/* This is a hack; we really need to figure these values out dynamically
 * 
 * Since 800 ns works very well with various HUB frequencies, such as
 * 360, 380, 390 and 400 MHZ, we use 800 ns rtc cycle time.
 *
 * Ralf: which clock rate is used to feed the counter?
 */
#define NSEC_PER_CYCLE		800
#define NSEC_PER_SEC		1000000000
#define CYCLES_PER_SEC		(NSEC_PER_SEC/NSEC_PER_CYCLE)
#define CYCLES_PER_JIFFY	(CYCLES_PER_SEC/HZ)

static unsigned long ct_cur;    /* What counter should be at next timer irq */

extern rwlock_t xtime_lock;

void rt_timer_interrupt(struct pt_regs *regs)
{
	int irq = 7;				/* XXX Assign number */

	write_lock(&xtime_lock);

again:
	LOCAL_HUB_S(PI_RT_PEND_A, 0);		/* Ack  */
	ct_cur += CYCLES_PER_JIFFY;
	LOCAL_HUB_S(PI_RT_COMPARE_A, ct_cur);

	if (LOCAL_HUB_L(PI_RT_COUNT) >= ct_cur)
		goto again;

	kstat.irqs[0][irq]++;
	do_timer(regs);

	write_unlock(&xtime_lock);
}

void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;

	read_lock_irqsave(&xtime_lock, flags);
	*tv = xtime;
	read_unlock_irqrestore(&xtime_lock, flags);
}

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq(&xtime_lock);
	xtime = *tv;
	time_state = TIME_BAD;
	time_maxerror = MAXPHASE;
	time_esterror = MAXPHASE;
	write_unlock_irq(&xtime_lock);
}

/* Includes for ioc3_init().  */
#include <linux/init.h>
#include <asm/sn/types.h>
#include <asm/sn/sn0/addrs.h>
#include <asm/sn/sn0/hubni.h>
#include <asm/sn/sn0/hubio.h>
#include <asm/sn/klconfig.h>
#include <asm/ioc3.h>
#include <asm/pci/bridge.h>

extern void ioc3_eth_init(void);

void __init time_init(void)
{
	unsigned int cpufreq;
	char *cpufreqstr;

	/* Is this timesource good enough?  Ok to assume that all CPUs have
	   this clockrate?  Are they 100% synchronously clocked?  */
	cpufreqstr = ArcGetEnvironmentVariable("cpufreq");
	if (cpufreqstr == NULL)
		panic("Cannot detect CPU clock rate");
	cpufreq = simple_strtoul(cpufreqstr, NULL, 10);
	printk("PROM says CPU clock is %dMHz\n", cpufreq);

	/* We didn't flush the TLB earlier since the ARC firmware depends on
	   it.  So do it now.  */
	flush_tlb_all();

	/* Don't worry about second CPU, it's disabled.  */
	LOCAL_HUB_S(PI_RT_EN_A, 1);
	LOCAL_HUB_S(PI_PROF_EN_A, 0);
	ct_cur = CYCLES_PER_JIFFY;
	LOCAL_HUB_S(PI_RT_COMPARE_A, ct_cur);
	LOCAL_HUB_S(PI_RT_COUNT, 0);
	LOCAL_HUB_S(PI_RT_PEND_A, 0);

	set_cp0_status(SRB_TIMOCLK, SRB_TIMOCLK);
}
