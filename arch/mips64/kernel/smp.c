/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000, 2001 Kanoj Sarcar
 * Copyright (C) 2000, 2001 Ralf Baechle
 * Copyright (C) 2000, 2001 Silicon Graphics, Inc.
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/sched.h>

#include <asm/atomic.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/hardirq.h>
#include <asm/softirq.h>
#include <asm/mmu_context.h>
#include <asm/irq.h>

/* The 'big kernel lock' */
spinlock_t kernel_flag = SPIN_LOCK_UNLOCKED;
int smp_threads_ready;	/* Not used */
atomic_t smp_commenced = ATOMIC_INIT(0);
struct cpuinfo_mips cpu_data[NR_CPUS];
int smp_num_cpus = 1;		/* Number that came online.  */
int __cpu_number_map[NR_CPUS];
int __cpu_logical_map[NR_CPUS];
cycles_t cacheflush_time;

static void smp_tune_scheduling (void)
{
}

void __init smp_boot_cpus(void)
{
	extern void allowboot(void);

	init_new_context(current, &init_mm);
	current->processor = 0;
	init_idle();
	smp_tune_scheduling();
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
	sendintr(cpu, DORESCHED);
}

/* Not really SMP stuff ... */
int setup_profiling_timer(unsigned int multiplier)
{
	return 0;
}

/*
 * This following are the global intr on off routines, copied almost
 * entirely from i386 code.
 */

int global_irq_holder = NO_PROC_ID;
spinlock_t global_irq_lock = SPIN_LOCK_UNLOCKED;

extern void show_stack(unsigned long* esp);

static void show(char * str)
{
	int i;
	int cpu = smp_processor_id();

	printk("\n%s, CPU %d:\n", str, cpu);
	printk("irq:  %d [",irqs_running());
	for(i=0;i < smp_num_cpus;i++)
		printk(" %d",local_irq_count(i));
	printk(" ]\nbh:   %d [",spin_is_locked(&global_bh_lock) ? 1 : 0);
	for(i=0;i < smp_num_cpus;i++)
		printk(" %d",local_bh_count(i));

	printk(" ]\nStack dumps:");
	for(i = 0; i < smp_num_cpus; i++) {
		if (i == cpu)
			continue;
		printk("\nCPU %d:",i);
		printk("Code not developed yet\n");
		/* show_stack(0); */
	}
	printk("\nCPU %d:",cpu);
	printk("Code not developed yet\n");
	/* show_stack(NULL); */
	printk("\n");
}

#define MAXCOUNT 		100000000
#define SYNC_OTHER_CORES(x)	udelay(x+1)

static inline void wait_on_irq(int cpu)
{
	int count = MAXCOUNT;

	for (;;) {

		/*
		 * Wait until all interrupts are gone. Wait
		 * for bottom half handlers unless we're
		 * already executing in one..
		 */
		if (!irqs_running())
			if (local_bh_count(cpu) || !spin_is_locked(&global_bh_lock))
				break;

		/* Duh, we have to loop. Release the lock to avoid deadlocks */
		spin_unlock(&global_irq_lock);

		for (;;) {
			if (!--count) {
				show("wait_on_irq");
				count = ~0;
			}
			__sti();
			SYNC_OTHER_CORES(cpu);
			__cli();
			if (irqs_running())
				continue;
			if (spin_is_locked(&global_irq_lock))
				continue;
			if (!local_bh_count(cpu) && spin_is_locked(&global_bh_lock))
				continue;
			if (spin_trylock(&global_irq_lock))
				break;
		}
	}
}

void synchronize_irq(void)
{
	if (irqs_running()) {
		/* Stupid approach */
		cli();
		sti();
	}
}

static inline void get_irqlock(int cpu)
{
	if (!spin_trylock(&global_irq_lock)) {
		/* do we already hold the lock? */
		if ((unsigned char) cpu == global_irq_holder)
			return;
		/* Uhhuh.. Somebody else got it. Wait.. */
		spin_lock(&global_irq_lock);
	}
	/*
	 * We also to make sure that nobody else is running
	 * in an interrupt context.
	 */
	wait_on_irq(cpu);

	/*
	 * Ok, finally..
	 */
	global_irq_holder = cpu;
}

void __global_cli(void)
{
	unsigned int flags;

	__save_flags(flags);
	if (flags & ST0_IE) {
		int cpu = smp_processor_id();
		__cli();
		if (!local_irq_count(cpu))
			get_irqlock(cpu);
	}
}

void __global_sti(void)
{
	int cpu = smp_processor_id();

	if (!local_irq_count(cpu))
		release_irqlock(cpu);
	__sti();
}

/*
 * SMP flags value to restore to:
 * 0 - global cli
 * 1 - global sti
 * 2 - local cli
 * 3 - local sti
 */
unsigned long __global_save_flags(void)
{
	int retval;
	int local_enabled;
	unsigned long flags;
	int cpu = smp_processor_id();

	__save_flags(flags);
	local_enabled = (flags & ST0_IE);
	/* default to local */
	retval = 2 + local_enabled;

	/* check for global flags if we're not in an interrupt */
	if (!local_irq_count(cpu)) {
		if (local_enabled)
			retval = 1;
		if (global_irq_holder == cpu)
			retval = 0;
	}
	return retval;
}

void __global_restore_flags(unsigned long flags)
{
	switch (flags) {
		case 0:
			__global_cli();
			break;
		case 1:
			__global_sti();
			break;
		case 2:
			__cli();
			break;
		case 3:
			__sti();
			break;
		default:
			printk("global_restore_flags: %08lx\n", flags);
	}
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
static volatile struct call_data_struct {
	void (*func) (void *info);
	void *info;
	atomic_t started;
	atomic_t finished;
	int wait;
} *call_data;

int smp_call_function (void (*func) (void *info), void *info, int retry, 
								int wait)
{
	struct call_data_struct data;
	int i, cpus = smp_num_cpus-1;
	static spinlock_t lock = SPIN_LOCK_UNLOCKED;

	if (cpus == 0)
		return 0;

	data.func = func;
	data.info = info;
	atomic_set(&data.started, 0);
	data.wait = wait;
	if (wait)
		atomic_set(&data.finished, 0);

	spin_lock_bh(&lock);
	call_data = &data;
	/* Send a message to all other CPUs and wait for them to respond */
	for (i = 0; i < smp_num_cpus; i++)
		if (smp_processor_id() != i)
			sendintr(i, DOCALL);

	/* Wait for response */
	/* FIXME: lock-up detection, backtrace on lock-up */
	while (atomic_read(&data.started) != cpus)
		barrier();

	if (wait)
		while (atomic_read(&data.finished) != cpus)
			barrier();
	spin_unlock_bh(&lock);
	return 0;
}

extern void smp_call_function_interrupt(int irq, void *d, struct pt_regs *r)
{
	void (*func) (void *info) = call_data->func;
	void *info = call_data->info;
	int wait = call_data->wait;

	/*
	 * Notify initiating CPU that I've grabbed the data and am
	 * about to execute the function.
	 */
	atomic_inc(&call_data->started);

	/*
	 * At this point the info structure may be out of scope unless wait==1.
	 */
	(*func)(info);
	if (wait)
		atomic_inc(&call_data->finished);
}
	

static void flush_tlb_all_ipi(void *info)
{
	_flush_tlb_all();
}

void flush_tlb_all(void)
{
	smp_call_function(flush_tlb_all_ipi, 0, 1, 1);
	_flush_tlb_all();
}

static void flush_tlb_mm_ipi(void *mm)
{
	_flush_tlb_mm((struct mm_struct *)mm);
}

/*
 * The following tlb flush calls are invoked when old translations are 
 * being torn down, or pte attributes are changing. For single threaded
 * address spaces, a new context is obtained on the current cpu, and tlb
 * context on other cpus are invalidated to force a new context allocation
 * at switch_mm time, should the mm ever be used on other cpus. For 
 * multithreaded address spaces, intercpu interrupts have to be sent.
 * Another case where intercpu interrupts are required is when the target
 * mm might be active on another cpu (eg debuggers doing the flushes on
 * behalf of debugees, kswapd stealing pages from another process etc).
 * Kanoj 07/00.
 */

void flush_tlb_mm(struct mm_struct *mm)
{
	if ((atomic_read(&mm->mm_users) != 1) || (current->mm != mm)) {
		smp_call_function(flush_tlb_mm_ipi, (void *)mm, 1, 1);
	} else {
		int i;
		for (i = 0; i < smp_num_cpus; i++)
			if (smp_processor_id() != i)
				CPU_CONTEXT(i, mm) = 0;
	}
	_flush_tlb_mm(mm);
}

struct flush_tlb_data {
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned long addr1;
	unsigned long addr2;
};

static void flush_tlb_range_ipi(void *info)
{
	struct flush_tlb_data *fd = (struct flush_tlb_data *)info;

	_flush_tlb_range(fd->mm, fd->addr1, fd->addr2);
}

void flush_tlb_range(struct mm_struct *mm, unsigned long start, unsigned long end)
{
	if ((atomic_read(&mm->mm_users) != 1) || (current->mm != mm)) {
		struct flush_tlb_data fd;

		fd.mm = mm;
		fd.addr1 = start;
		fd.addr2 = end;
		smp_call_function(flush_tlb_range_ipi, (void *)&fd, 1, 1);
	} else {
		int i;
		for (i = 0; i < smp_num_cpus; i++)
			if (smp_processor_id() != i)
				CPU_CONTEXT(i, mm) = 0;
	}
	_flush_tlb_range(mm, start, end);
}

static void flush_tlb_page_ipi(void *info)
{
	struct flush_tlb_data *fd = (struct flush_tlb_data *)info;

	_flush_tlb_page(fd->vma, fd->addr1);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	if ((atomic_read(&vma->vm_mm->mm_users) != 1) || (current->mm != vma->vm_mm)) {
		struct flush_tlb_data fd;

		fd.vma = vma;
		fd.addr1 = page;
		smp_call_function(flush_tlb_page_ipi, (void *)&fd, 1, 1);
	} else {
		int i;
		for (i = 0; i < smp_num_cpus; i++)
			if (smp_processor_id() != i)
				CPU_CONTEXT(i, vma->vm_mm) = 0;
	}
	_flush_tlb_page(vma, page);
}
