/*
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
 *
 * Copyright (C) 2000, 2001 Kanoj Sarcar
 * Copyright (C) 2000, 2001 Ralf Baechle
 * Copyright (C) 2000, 2001 Silicon Graphics, Inc.
 * Copyright (C) 2000, 2001 Broadcom Corporation
 */ 
#include <linux/config.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/timex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/atomic.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/hardirq.h>
#include <asm/softirq.h>
#include <asm/mmu_context.h>
#include <asm/delay.h>
#include <asm/smp.h>

/* Ze Big Kernel Lock! */
spinlock_t kernel_flag = SPIN_LOCK_UNLOCKED;
int smp_threads_ready;
int smp_num_cpus = 1;			/* Number that came online.  */
cpumask_t cpu_online_map;		/* Bitmask of currently online CPUs */
int global_irq_holder = NO_PROC_ID;
spinlock_t global_irq_lock = SPIN_LOCK_UNLOCKED;
struct cpuinfo_mips cpu_data[NR_CPUS];
void (*volatile smp_cpu0_finalize)(void) = NULL;

static atomic_t cpus_booted = ATOMIC_INIT(0);


/* These are defined by the board-specific code. */

/*
 * Cause the function described by call_data to be executed on the passed
 * cpu.  When the function has finished, increment the finished field of
 * call_data.
 */
void core_send_ipi(int cpu, unsigned int action);

/*
 * Clear all undefined state in the cpu, set up sp and gp to the passed
 * values, and kick the cpu into smp_bootstrap(); 
 */
void prom_boot_secondary(int cpu, unsigned long sp, unsigned long gp);

/*
 *  After we've done initial boot, this function is called to allow the
 *  board code to clean up state, if needed 
 */
void prom_init_secondary(void);

/*
 * Do whatever setup needs to be done for SMP at the board level.  Return
 * the number of cpus in the system, including this one
 */
int prom_setup_smp(void);

/*
 * Hook for doing final board-specific setup after the generic smp setup
 * is done
 */
asmlinkage int start_secondary(void)
{
	prom_init_secondary();

	/* Do stuff that trap_init() did for the first processor */
	clear_cp0_status(ST0_BEV);
	if (mips_cpu.options & MIPS_CPU_DIVEC) {
		set_cp0_cause(CAUSEF_IV);
	}
	/* XXX parity protection should be folded in here when it's converted to
	   an option instead of something based on .cputype */

	write_32bit_cp0_register(CP0_CONTEXT, smp_processor_id()<<23);
	pgd_current[smp_processor_id()] = init_mm.pgd;
	cpu_data[smp_processor_id()].udelay_val = loops_per_jiffy;
	cpu_data[smp_processor_id()].asid_cache = ASID_FIRST_VERSION;
	prom_smp_finish();
	printk("Slave cpu booted successfully\n");
	atomic_inc(&cpus_booted);
	cpu_idle();
	return 0;
}

void __init smp_boot_cpus(void)
{
	int i;

	smp_num_cpus = prom_setup_smp();
	init_new_context(current, &init_mm);
	current->processor = 0;
	cpu_data[0].udelay_val = loops_per_jiffy;
	cpu_data[0].asid_cache = ASID_FIRST_VERSION;
	atomic_set(&cpus_booted, 1);  /* Master CPU is already booted... */
	init_idle();
	for (i = 1; i < smp_num_cpus; i++) {
		struct task_struct *p;
		struct pt_regs regs;
		printk("Starting CPU %d... ", i);

		/* Spawn a new process normally.  Grab a pointer to
		   its task struct so we can mess with it */
		do_fork(CLONE_VM|CLONE_PID, 0, &regs, 0);
		p = init_task.prev_task;

		/* Schedule the first task manually */
		p->processor = i;
		p->cpus_runnable = 1 << i; /* we schedule the first task manually */

		/* Attach to the address space of init_task. */
		atomic_inc(&init_mm.mm_count);
		p->active_mm = &init_mm;
		init_tasks[i] = p;

		del_from_runqueue(p);
		unhash_process(p);

		prom_boot_secondary(i,
				    (unsigned long)p + KERNEL_STACK_SIZE - 32,
				    (unsigned long)p);

#if 0

		/* This is copied from the ip-27 code in the mips64 tree */

		struct task_struct *p;

		/*
		 * The following code is purely to make sure
		 * Linux can schedule processes on this slave.
		 */
		kernel_thread(0, NULL, CLONE_PID);
		p = init_task.prev_task;
		sprintf(p->comm, "%s%d", "Idle", i);
		init_tasks[i] = p;
		p->processor = i;
		p->cpus_runnable = 1 << i; /* we schedule the first task manually *
		del_from_runqueue(p);
		unhash_process(p);
		/* Attach to the address space of init_task. */
		atomic_inc(&init_mm.mm_count);
		p->active_mm = &init_mm;
		prom_boot_secondary(i, 
				    (unsigned long)p + KERNEL_STACK_SIZE - 32,
				    (unsigned long)p);
#endif
	}

	/* Wait for everyone to come up */
	while (atomic_read(&cpus_booted) != smp_num_cpus);
	smp_threads_ready = 1;
}

void __init smp_commence(void)
{
	/* Not sure what to do here yet */
}

static void reschedule_this_cpu(void *dummy)
{
	current->need_resched = 1;
}

void smp_send_reschedule(int cpu)
{
	smp_call_function(reschedule_this_cpu, NULL, 0, 0);
}

static spinlock_t call_lock = SPIN_LOCK_UNLOCKED;

struct call_data_struct *call_data;

/*
 * The caller of this wants the passed function to run on every cpu.  If wait
 * is set, wait until all cpus have finished the function before returning.
 * The lock is here to protect the call structure.
 */
int smp_call_function (void (*func) (void *info), void *info, int retry, 
								int wait)
{
	struct call_data_struct data;
	int cpus = smp_num_cpus - 1;
	int cpu = smp_processor_id();
	int i;

	if (!cpus)
		return 0;

	data.func = func;
	data.info = info;
	atomic_set(&data.started, 0);
	data.wait = wait;
	if (wait)
		atomic_set(&data.finished, 0);

	spin_lock_bh(&call_lock);
	call_data = &data;
	for (i = 0; i < smp_num_cpus; i++) {
		if (i != cpu) {
			/* Call the board specific routine */
			core_send_ipi(i, SMP_CALL_FUNCTION);
		}
	}

	/* Wait for response */
	while (atomic_read(&data.started) != cpus)
		barrier();

	if (wait)
		while(atomic_read(&data.finished) != cpus)
			barrier();
	spin_unlock_bh(&call_lock);

	return 0;
}

void synchronize_irq(void)
{
	if (irqs_running()) {
		/* Stupid approach */
		cli();
		sti();
	}
}

static void stop_this_cpu(void *dummy)
{
	int cpu = smp_processor_id();
	if (cpu)
		for (;;);		/* XXX Use halt like i386 */
	else {
		/* XXXKW this isn't quite there yet */
		while (!smp_cpu0_finalize) ;
				smp_cpu0_finalize();
		}
}

void smp_send_stop(void)
{
	smp_call_function(stop_this_cpu, NULL, 1, 0);
	smp_num_cpus = 1;
}

/* Not really SMP stuff ... */
int setup_profiling_timer(unsigned int multiplier)
{
	return 0;
}


/*
 * Most of this code is take from the mips64 tree (ip27-irq.c).  It's virtually
 * identical to the i386 implentation in arh/i386/irq.c, with translations for
 * the interrupt enable bit
 */

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
				printk("Count spun out.  Huh?\n");
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


/*
 * A global "cli()" while in an interrupt context
 * turns into just a local cli(). Interrupts
 * should use spinlocks for the (very unlikely)
 * case that they ever want to protect against
 * each other.
 *
 * If we already have local interrupts disabled,
 * this will not turn a local disable into a
 * global one (problems with spinlocks: this makes
 * save_flags+cli+sti usable inside a spinlock).
 */
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

static void flush_tlb_all_ipi(void *info)
{
	local_flush_tlb_all();
}

void flush_tlb_all(void)
{
	smp_call_function(flush_tlb_all_ipi, 0, 1, 1);
	local_flush_tlb_all();
}

static void flush_tlb_mm_ipi(void *mm)
{
	local_flush_tlb_mm((struct mm_struct *)mm);
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
	local_flush_tlb_mm(mm);
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

	local_flush_tlb_range(fd->mm, fd->addr1, fd->addr2);
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
	local_flush_tlb_range(mm, start, end);
}

static void flush_tlb_page_ipi(void *info)
{
	struct flush_tlb_data *fd = (struct flush_tlb_data *)info;

	local_flush_tlb_page(fd->vma, fd->addr1);
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
	local_flush_tlb_page(vma, page);
}

EXPORT_SYMBOL(flush_tlb_page);
EXPORT_SYMBOL(cpu_data);
EXPORT_SYMBOL(synchronize_irq);
EXPORT_SYMBOL(kernel_flag);
EXPORT_SYMBOL(__global_sti);
EXPORT_SYMBOL(__global_cli);
EXPORT_SYMBOL(__global_save_flags);
EXPORT_SYMBOL(__global_restore_flags);
