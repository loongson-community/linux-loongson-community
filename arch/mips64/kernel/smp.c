#include <linux/config.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/sched.h>

#include <asm/atomic.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/hardirq.h>

#ifdef CONFIG_SGI_IP27

#include <asm/sn/arch.h>
#include <asm/sn/intr.h>
#include <asm/sn/addrs.h>
#include <asm/sn/agent.h>

#define DOACTION	0xab

static void sendintr(int destid, unsigned char status)
{
	int level;

#if (CPUS_PER_NODE == 2)
	/*
	 * CPU slice A gets level CPU_ACTION_A
	 * CPU slice B gets level CPU_ACTION_B
	 */
	if (status == DOACTION)
		level = CPU_ACTION_A + cputoslice(destid);
	else	/* DOTLBACTION */
		level = N_INTPEND_BITS + TLB_INTR_A + cputoslice(destid);

	/*
	 * Convert the compact hub number to the NASID to get the correct
	 * part of the address space.  Then set the interrupt bit associated
	 * with the CPU we want to send the interrupt to.
	 */
	REMOTE_HUB_SEND_INTR(COMPACT_TO_NASID_NODEID(cputocnode(destid)), level);
#else
	<< Bomb!  Must redefine this for more than 2 CPUS. >>
#endif
}

#endif /* CONFIG_SGI_IP27 */

/* The 'big kernel lock' */
spinlock_t kernel_flag = SPIN_LOCK_UNLOCKED;
int smp_threads_ready = 0;	/* Not used */
atomic_t smp_commenced = ATOMIC_INIT(0);
struct cpuinfo_mips cpu_data[NR_CPUS];
int smp_num_cpus;		/* Number that came online.  */
int __cpu_number_map[NR_CPUS];
int __cpu_logical_map[NR_CPUS];
cycles_t cacheflush_time;

static void smp_tune_scheduling (void)
{
}

void __init smp_boot_cpus(void)
{
	global_irq_holder = 0;
	current->processor = 0;
	init_idle();
	smp_tune_scheduling();
	smp_num_cpus = 1;		/* for now */
	allowboot();
}

void __init smp_commence(void)
{
	wmb();
	atomic_set(&smp_commenced,1);
}

static void stop_this_cpu(void *dummy)
{
	/*
	 * Remove this CPU
	 */
	for (;;);
}

void smp_send_stop(void)
{
	smp_call_function(stop_this_cpu, NULL, 1, 0);
	smp_num_cpus = 1;
}

/*
 * this function sends a 'reschedule' IPI to another CPU.
 * it goes straight through and wastes no time serializing
 * anything. Worst case is that we lose a reschedule ...
 */
void smp_send_reschedule(int cpu)
{
	panic("smp_send_reschedule\n");
}

/* Not really SMP stuff ... */
int setup_profiling_timer(unsigned int multiplier)
{
}

/*
 * Run a function on all other CPUs.
 *  <func>      The function to run. This must be fast and non-blocking.
 *  <info>      An arbitrary pointer to pass to the function.
 *  <retry>     If true, keep retrying until ready.
 *  <wait>      If true, wait until function has completed on other CPUs.
 *  [RETURNS]   0 on success, else a negative status code.
 *
 * Does not return until remote CPUs are nearly ready to execute <func>
 * or are or have executed.
 */
int
smp_call_function (void (*func) (void *info), void *info, int retry, int wait)
{
	/* XXX - kinda important ;-)  */
	panic("smp_call_function\n");
}

void flush_tlb_others (unsigned long cpumask, struct mm_struct *mm, 
						unsigned long va)
{
	panic("flush_tlb_others\n");
}
