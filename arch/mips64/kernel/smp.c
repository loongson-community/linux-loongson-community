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

/* The 'big kernel lock' */
spinlock_t kernel_flag = SPIN_LOCK_UNLOCKED;

int smp_threads_ready = 0;

static void smp_tune_scheduling (void)
{
}

void __init smp_boot_cpus(void)
{
	global_irq_holder = 0;
	current->processor = 0;
	init_idle();
	smp_tune_scheduling();
}

static atomic_t smp_commenced = ATOMIC_INIT(0);

struct cpuinfo_mips cpu_data[NR_CPUS];

int smp_num_cpus = 1;		/* Number that came online.  */

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
}

int __cpu_number_map[NR_CPUS];
int __cpu_logical_map[NR_CPUS];

cycles_t cacheflush_time;

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
}
