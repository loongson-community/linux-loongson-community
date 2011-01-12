/*
 * Based on Ocelot Linux port, which is
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
 *
 * Copyright 2003 ICT CAS
 * Author: Michael Guo <guoyi@ict.ac.cn>
 *
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 * Copyright (C) 2009 Lemote Inc.
 * Author: Wu Zhangjin, wuzhangjin@gmail.com
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/module.h>

#include <asm/addrspace.h>
#include <asm/bootinfo.h>

#include <loongson.h>

unsigned long cpu_clock_freq =
	((CONFIG_CPU_CLOCK_FREQ_LEVEL * CONFIG_CPU_CLOCK_FREQ) >> 3) * 1000;
EXPORT_SYMBOL(cpu_clock_freq);
unsigned long memsize = CONFIG_LOW_MEMSIZE;
unsigned long highmemsize = CONFIG_MEMSIZE - CONFIG_LOW_MEMSIZE;

#define parse_even_earlier(res, option, p)				\
do {									\
	if (strncmp(option, (char *)p, strlen(option)) == 0)		\
		strict_strtol((char *)p + strlen(option"="), 10, &res);	\
} while (0)

void __init prom_init_env(void)
{
#ifndef CONFIG_ENVVAR_OVERRIDE
	/* PMON passes arguments in 32bit pointers */
	int *_prom_envp;
	long l;
	/* Who will put the env vars at address 0? */
	if (fw_arg2 != 0) {
		/* firmware arguments are initialized in head.S */
		_prom_envp = (int *)fw_arg2;
		l = (long)*_prom_envp;
		while (l != 0) {
			parse_even_earlier(cpu_clock_freq, "cpuclock", l);
			parse_even_earlier(memsize, "memsize", l);
			parse_even_earlier(highmemsize, "highmemsize", l);
			_prom_envp++;
			l = (long)*_prom_envp;
		}
	}

	cpu_clock_freq = ((CONFIG_CPU_CLOCK_FREQ_LEVEL *
				(cpu_clock_freq / 1000)) >> 3) * 1000;
#endif	/* CONFIG_ENVVAR_OVERRIDE */

	pr_info("cpuclock=%ld, memsize=%ld, highmemsize=%ld\n",
			cpu_clock_freq, memsize, highmemsize);
}

static int __init init_cpu_freq(void)
{
	LOONGSON_SET_CPUFREQ(CONFIG_CPU_CLOCK_FREQ_LEVEL - 1);
	return 0;
}
/*
 * To get fastest booting, only set cpu freq in the last stage or delay this to
 * the user-space.
 */
#if CONFIG_CPU_CLOCK_FREQ_LEVEL < 8
late_initcall(init_cpu_freq);
#else
early_initcall(init_cpu_freq);
#endif
