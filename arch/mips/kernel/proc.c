/*
 *  linux/arch/mips/kernel/proc.c
 *
 *  Copyright (C) 1995, 1996, 2001  Ralf Baechle
 *  Copyright (C) 2001  MIPS Technologies, Inc.
 */
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/mipsregs.h>
#include <asm/processor.h>
#include <asm/watch.h>

unsigned int vced_count, vcei_count;

#ifndef CONFIG_CPU_HAS_LLSC
unsigned long ll_ops, sc_ops;
#endif

static const char *cpu_name[] = CPU_NAMES;

static int show_cpuinfo(struct seq_file *m, void *v)
{
	unsigned int version = mips_cpu.processor_id;
	unsigned int fp_vers = mips_cpu.fpu_id;
	unsigned long n = (unsigned long) v - 1;
	char fmt [64];

#ifdef CONFIG_SMP
	if (!(cpu_online_map & (1<<n)))
		return 0;
#endif

	seq_printf(m, "processor\t\t: %ld\n", n);
	sprintf(fmt, "cpu model\t\t: %%s V%%d.%%d%s\n",
	        (mips_cpu.options & MIPS_CPU_FPU) ? "  FPU V%d.%d" : "");
	seq_printf(m, fmt, cpu_name[mips_cpu.cputype <= CPU_LAST ?
	                            mips_cpu.cputype : CPU_UNKNOWN],
	                           (version >> 4) & 0x0f, version & 0x0f,
	                           (fp_vers >> 4) & 0x0f, fp_vers & 0x0f);
	seq_printf(m, "BogoMIPS\t\t: %lu.%02lu\n",
	              loops_per_jiffy / (500000/HZ),
	              (loops_per_jiffy / (5000/HZ)) % 100);
	seq_printf(m, "wait instruction\t: %s\n", cpu_wait ? "yes" : "no");
	seq_printf(m, "microsecond timers\t: %s\n",
	              (mips_cpu.options & MIPS_CPU_COUNTER) ? "yes" : "no");
	seq_printf(m, "extra interrupt vector\t: %s\n",
	              (mips_cpu.options & MIPS_CPU_DIVEC) ? "yes" : "no");
	seq_printf(m, "hardware watchpoint\t: %s\n",
	              watch_available ? "yes" : "no");

	sprintf(fmt, "VCE%%c exceptions\t\t: %s\n",
	        (mips_cpu.options & MIPS_CPU_VCE) ? "%d" : "not available");
	seq_printf(m, fmt, 'D', vced_count);
	seq_printf(m, fmt, 'I', vcei_count);

#ifndef CONFIG_CPU_HAS_LLSC
	seq_printf(m, "ll emulations\t\t: %lu\n", ll_ops);
	seq_printf(m, "sc emulations\t\t: %lu\n", sc_ops);
#endif

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	unsigned long i = *pos;

	return i < NR_CPUS ? (void *) (i + 1) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
	start:	c_start,
	next:	c_next,
	stop:	c_stop,
	show:	show_cpuinfo,
};

void init_irq_proc(void)
{
	/* Nothing, for now.  */
}
